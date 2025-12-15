import signal
import sys
import time

import RPi.GPIO as GPIO


running = True


def handle_signal(signum, frame):
    global running
    running = False


def us_to_duty(pulse_us: int, period_hz: float = 50.0) -> float:
    period_us = 1_000_000.0 / period_hz
    return (pulse_us / period_us) * 100.0


def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <bcm_pin>")
        sys.exit(1)

    pin = int(sys.argv[1])

    signal.signal(signal.SIGINT, handle_signal)
    signal.signal(signal.SIGTERM, handle_signal)

    GPIO.setmode(GPIO.BCM)
    GPIO.setup(pin, GPIO.OUT)

    pwm = GPIO.PWM(pin, 50)  # 50 Hz servo signal
    pwm.start(0)

    min_us = 1000
    max_us = 2000
    step_us = 10
    step_sleep = 0.025  # 25 ms -> ~5 s full sweep (2.5 s up, 2.5 s down)

    value = min_us
    direction = 1

    try:
        while running:
            duty = us_to_duty(value)
            pwm.ChangeDutyCycle(duty)

            value += direction * step_us
            if value >= max_us:
                value = max_us
                direction = -1
            elif value <= min_us:
                value = min_us
                direction = 1

            time.sleep(step_sleep)
    finally:
        pwm.stop()
        GPIO.cleanup(pin)
        GPIO.cleanup()


if __name__ == "__main__":
    main()
