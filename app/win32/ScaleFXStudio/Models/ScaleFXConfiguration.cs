using System.ComponentModel;
using YamlDotNet.Serialization;

namespace ScaleFXStudio.Models;

/// <summary>
/// Root configuration for ScaleFX Hub system
/// </summary>
public class ScaleFXConfiguration
{
    [Category("Engine FX")]
    [Description("Engine sound effects configuration")]
    [YamlMember(Alias = "engine_fx")]
    public EngineFxConfig? EngineFx { get; set; }

    [Category("Gun FX")]
    [Description("Gun effects configuration")]
    [YamlMember(Alias = "gun_fx")]
    public GunFxConfig? GunFx { get; set; }
}

public class EngineFxConfig
{
    [Category("General")]
    [Description("Enable or disable engine effects")]
    [YamlMember(Alias = "enabled")]
    public bool Enabled { get; set; } = true;

    [Category("Engine Toggle")]
    [Description("Engine toggle PWM input configuration")]
    [YamlMember(Alias = "engine_toggle")]
    public PwmInputConfig? EngineToggle { get; set; }

    [Category("Sounds")]
    [Description("Engine sound files configuration")]
    [YamlMember(Alias = "sounds")]
    public EngineSoundsConfig? Sounds { get; set; }
}

public class PwmInputConfig
{
    [Category("PWM")]
    [Description("GPIO pin number for PWM input")]
    [YamlMember(Alias = "pin")]
    public int Pin { get; set; }

    [Category("PWM")]
    [Description("PWM threshold in microseconds")]
    [YamlMember(Alias = "threshold_us")]
    public int ThresholdUs { get; set; } = 1500;
}

public class EngineSoundsConfig
{
    [Category("Sounds")]
    [Description("Engine starting sound")]
    [YamlMember(Alias = "starting")]
    public SoundFileConfig? Starting { get; set; }

    [Category("Sounds")]
    [Description("Engine running loop sound")]
    [YamlMember(Alias = "running")]
    public SoundFileConfig? Running { get; set; }

    [Category("Sounds")]
    [Description("Engine stopping sound")]
    [YamlMember(Alias = "stopping")]
    public SoundFileConfig? Stopping { get; set; }

    [Category("Transitions")]
    [Description("Sound transition timing")]
    [YamlMember(Alias = "transitions")]
    public TransitionsConfig? Transitions { get; set; }
}

public class SoundFileConfig
{
    [Category("Sound")]
    [Description("Path to sound file")]
    [Editor(typeof(System.Windows.Forms.Design.FileNameEditor), typeof(System.Drawing.Design.UITypeEditor))]
    [YamlMember(Alias = "file")]
    public string? File { get; set; }
}

public class TransitionsConfig
{
    [Category("Timing")]
    [Description("Offset in milliseconds when starting")]
    [YamlMember(Alias = "starting_offset_ms")]
    public int StartingOffsetMs { get; set; }

    [Category("Timing")]
    [Description("Offset in milliseconds when stopping")]
    [YamlMember(Alias = "stopping_offset_ms")]
    public int StoppingOffsetMs { get; set; }
}

public class GunFxConfig
{
    [Category("General")]
    [Description("Enable or disable gun effects")]
    [YamlMember(Alias = "enabled")]
    public bool Enabled { get; set; } = true;

    [Category("Trigger")]
    [Description("Gun trigger PWM input configuration")]
    [YamlMember(Alias = "trigger")]
    public TriggerConfig? Trigger { get; set; }

    [Category("Smoke")]
    [Description("Smoke generator configuration")]
    [YamlMember(Alias = "smoke")]
    public SmokeConfig? Smoke { get; set; }

    [Category("Turret Control")]
    [Description("Turret servo control configuration")]
    [YamlMember(Alias = "turret_control")]
    public TurretControlConfig? TurretControl { get; set; }

    [Category("Rate of Fire")]
    [Description("Rate of fire configurations")]
    [YamlMember(Alias = "rate_of_fire")]
    public List<RateOfFireConfig>? RateOfFire { get; set; }
}

public class TriggerConfig
{
    [Category("Trigger")]
    [Description("GPIO pin for trigger PWM input")]
    [YamlMember(Alias = "pin")]
    public int Pin { get; set; }
}

public class SmokeConfig
{
    [Category("General")]
    [Description("Enable smoke generator")]
    [YamlMember(Alias = "enabled")]
    public bool Enabled { get; set; } = true;

    [Category("Heater")]
    [Description("GPIO pin for heater toggle PWM input")]
    [YamlMember(Alias = "heater_toggle_pin")]
    public int HeaterTogglePin { get; set; }

    [Category("Heater")]
    [Description("PWM threshold for heater activation (µs)")]
    [YamlMember(Alias = "heater_pwm_threshold_us")]
    public int HeaterPwmThresholdUs { get; set; } = 1500;
}

public class TurretControlConfig
{
    [Category("Pitch")]
    [Description("Pitch axis servo configuration")]
    [YamlMember(Alias = "pitch")]
    public ServoConfig? Pitch { get; set; }

    [Category("Yaw")]
    [Description("Yaw axis servo configuration")]
    [YamlMember(Alias = "yaw")]
    public ServoConfig? Yaw { get; set; }
}

public class ServoConfig
{
    [Category("General")]
    [Description("Enable this servo axis")]
    [YamlMember(Alias = "enabled")]
    public bool Enabled { get; set; } = true;

    [Category("General")]
    [Description("Servo ID (1-3)")]
    [YamlMember(Alias = "servo_id")]
    public int ServoId { get; set; } = 1;

    [Category("PWM Input")]
    [Description("GPIO pin for PWM input")]
    [YamlMember(Alias = "pwm_pin")]
    public int PwmPin { get; set; }

    [Category("PWM Input")]
    [Description("Minimum input PWM (µs)")]
    [YamlMember(Alias = "input_min_us")]
    public int InputMinUs { get; set; } = 1000;

    [Category("PWM Input")]
    [Description("Maximum input PWM (µs)")]
    [YamlMember(Alias = "input_max_us")]
    public int InputMaxUs { get; set; } = 2000;

    [Category("PWM Output")]
    [Description("Minimum output PWM (µs)")]
    [YamlMember(Alias = "output_min_us")]
    public int OutputMinUs { get; set; } = 1000;

    [Category("PWM Output")]
    [Description("Maximum output PWM (µs)")]
    [YamlMember(Alias = "output_max_us")]
    public int OutputMaxUs { get; set; } = 2000;

    [Category("Motion")]
    [Description("Maximum speed (µs/sec)")]
    [YamlMember(Alias = "max_speed_us_per_sec")]
    public double MaxSpeedUsPerSec { get; set; } = 500.0;

    [Category("Motion")]
    [Description("Maximum acceleration (µs/sec²)")]
    [YamlMember(Alias = "max_accel_us_per_sec2")]
    public double MaxAccelUsPerSec2 { get; set; } = 2000.0;

    [Category("Motion")]
    [Description("Servo update rate (Hz)")]
    [YamlMember(Alias = "update_rate_hz")]
    public int UpdateRateHz { get; set; } = 50;
}

public class RateOfFireConfig
{
    [Category("Rate")]
    [Description("Display name for this rate")]
    [YamlMember(Alias = "name")]
    public string? Name { get; set; }

    [Category("Rate")]
    [Description("Rounds per minute")]
    [YamlMember(Alias = "rpm")]
    public int Rpm { get; set; }

    [Category("Rate")]
    [Description("PWM threshold to activate this rate (µs)")]
    [YamlMember(Alias = "pwm_threshold_us")]
    public int PwmThresholdUs { get; set; }

    [Category("Sound")]
    [Description("Sound file for this firing rate")]
    [Editor(typeof(System.Windows.Forms.Design.FileNameEditor), typeof(System.Drawing.Design.UITypeEditor))]
    [YamlMember(Alias = "sound")]
    public string? Sound { get; set; }

    public override string ToString() => Name ?? $"{Rpm} RPM";
}
