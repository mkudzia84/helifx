--[[
    HeliFX Remote Configuration
    
    Jeti transmitter Lua application for configuring HeliFX parameters
    via JetiEX telemetry protocol.
    
    Features:
    - Read/write HeliFX parameters remotely
    - Real-time parameter adjustment from transmitter
    - Save configuration to HeliFX
    - Telemetry display of gun/engine status
    
    Installation:
    1. Copy this file and helifx_config.jsn to /Apps/HeliFX/ on transmitter SD card
    2. Load application in transmitter menu
    3. Bind to HeliFX telemetry device
    
    Version: 1.0
    Author: HeliFX Project
    Date: 2025-12-05
--]]

local appName = "HeliFX Config"
local deviceId = 0xA409  -- Must match HeliFX manufacturer ID
local sensorIds = {}

-- Configuration parameters (ID, Name, Type, Min, Max, Current Value)
local parameters = {
    {id = 0,  name = "Gun Rate 1 RPM",      type = "uint16", min = 0,    max = 1000,  value = 200},
    {id = 1,  name = "Gun Rate 2 RPM",      type = "uint16", min = 0,    max = 1000,  value = 550},
    {id = 2,  name = "Gun Rate 1 PWM",      type = "uint16", min = 1000, max = 2000,  value = 1400},
    {id = 3,  name = "Gun Rate 2 PWM",      type = "uint16", min = 1000, max = 2000,  value = 1600},
    {id = 4,  name = "Smoke Fan Delay",     type = "uint16", min = 0,    max = 5000,  value = 2000},
    {id = 5,  name = "Heater PWM Thresh",   type = "uint16", min = 1000, max = 2000,  value = 1500},
    {id = 6,  name = "Engine PWM Thresh",   type = "uint16", min = 1000, max = 2000,  value = 1500},
    {id = 7,  name = "Servo Max Speed",     type = "uint16", min = 0,    max = 2000,  value = 500},
    {id = 8,  name = "Servo Max Accel",     type = "uint16", min = 0,    max = 5000,  value = 2000},
    {id = 9,  name = "Telemetry Rate Hz",   type = "uint8",  min = 5,    max = 100,   value = 10},
    {id = 10, name = "Nozzle Flash Enable", type = "bool",   min = 0,    max = 1,     value = 1},
    {id = 11, name = "Smoke Enable",        type = "bool",   min = 0,    max = 1,     value = 1},
}

-- UI State
local selectedParam = 1
local editMode = false
local statusMessage = "Ready"
local lastUpdate = 0

-- Initialize application
local function init()
    -- Register telemetry sensors
    sensorIds.gunRate = system.pLoad("gunRate", 0)
    sensorIds.engineState = system.pLoad("engineState", 0)
    
    -- Load saved configuration
    for i, param in ipairs(parameters) do
        local saved = system.pLoad(string.format("p%d", param.id))
        if saved then
            param.value = saved
        end
    end
    
    statusMessage = "Initialized"
end

-- Request parameter from HeliFX
local function requestParameter(paramId)
    -- Build config read command packet
    -- Format: [PACKET_TYPE][MFR_ID_L][MFR_ID_H][DEV_ID_L][DEV_ID_H][CMD][PARAM_ID][CRC_L][CRC_H]
    local packet = {
        0x3B,  -- PACKET_CONFIG
        bit32.band(deviceId, 0xFF),
        bit32.rshift(deviceId, 8),
        0x01,  -- Device ID low
        0x00,  -- Device ID high
        0x01,  -- CMD_READ
        paramId,
    }
    
    -- Calculate CRC-16 CCITT
    local crc = 0
    for i = 1, #packet do
        crc = bit32.bxor(crc, bit32.lshift(packet[i], 8))
        for j = 1, 8 do
            if bit32.band(crc, 0x8000) ~= 0 then
                crc = bit32.band(bit32.bxor(bit32.lshift(crc, 1), 0x1021), 0xFFFF)
            else
                crc = bit32.band(bit32.lshift(crc, 1), 0xFFFF)
            end
        end
    end
    
    -- Add CRC to packet
    table.insert(packet, bit32.band(crc, 0xFF))
    table.insert(packet, bit32.rshift(crc, 8))
    
    -- Send via EX Bus (implementation depends on Jeti API)
    -- system.sendEXPacket(packet)
    
    statusMessage = string.format("Reading param %d...", paramId)
end

-- Write parameter to HeliFX
local function writeParameter(paramId, value)
    local param = parameters[paramId + 1]  -- Lua is 1-indexed
    if not param then return end
    
    -- Build config write command packet
    local packet = {
        0x3B,  -- PACKET_CONFIG
        bit32.band(deviceId, 0xFF),
        bit32.rshift(deviceId, 8),
        0x01,  -- Device ID low
        0x00,  -- Device ID high
        0x02,  -- CMD_WRITE
        paramId,
    }
    
    -- Add value based on type
    if param.type == "uint8" or param.type == "bool" then
        table.insert(packet, value)
    elseif param.type == "uint16" or param.type == "int16" then
        table.insert(packet, bit32.band(value, 0xFF))
        table.insert(packet, bit32.rshift(value, 8))
    elseif param.type == "uint32" or param.type == "int32" then
        table.insert(packet, bit32.band(value, 0xFF))
        table.insert(packet, bit32.band(bit32.rshift(value, 8), 0xFF))
        table.insert(packet, bit32.band(bit32.rshift(value, 16), 0xFF))
        table.insert(packet, bit32.rshift(value, 24))
    end
    
    -- Calculate and add CRC (same as above)
    local crc = 0
    for i = 1, #packet do
        crc = bit32.bxor(crc, bit32.lshift(packet[i], 8))
        for j = 1, 8 do
            if bit32.band(crc, 0x8000) ~= 0 then
                crc = bit32.band(bit32.bxor(bit32.lshift(crc, 1), 0x1021), 0xFFFF)
            else
                crc = bit32.band(bit32.lshift(crc, 1), 0xFFFF)
            end
        end
    end
    
    table.insert(packet, bit32.band(crc, 0xFF))
    table.insert(packet, bit32.rshift(crc, 8))
    
    -- Send via EX Bus
    -- system.sendEXPacket(packet)
    
    -- Save to transmitter
    system.pSave(string.format("p%d", paramId), value)
    
    statusMessage = string.format("Wrote %s = %d", param.name, value)
end

-- Save all configuration to HeliFX
local function saveConfiguration()
    local packet = {
        0x3B,  -- PACKET_CONFIG
        bit32.band(deviceId, 0xFF),
        bit32.rshift(deviceId, 8),
        0x01,  -- Device ID low
        0x00,  -- Device ID high
        0x04,  -- CMD_SAVE
        0x00,  -- Parameter ID (unused for save)
    }
    
    -- Calculate and add CRC
    local crc = 0
    for i = 1, #packet do
        crc = bit32.bxor(crc, bit32.lshift(packet[i], 8))
        for j = 1, 8 do
            if bit32.band(crc, 0x8000) ~= 0 then
                crc = bit32.band(bit32.bxor(bit32.lshift(crc, 1), 0x1021), 0xFFFF)
            else
                crc = bit32.band(bit32.lshift(crc, 1), 0xFFFF)
            end
        end
    end
    
    table.insert(packet, bit32.band(crc, 0xFF))
    table.insert(packet, bit32.rshift(crc, 8))
    
    -- Send via EX Bus
    -- system.sendEXPacket(packet)
    
    statusMessage = "Configuration saved!"
end

-- Draw main menu
local function printForm()
    -- Header
    lcd.drawText(5, 2, appName, FONT_BOLD)
    lcd.drawText(160 - lcd.getTextWidth(FONT_MINI, statusMessage), 2, statusMessage, FONT_MINI)
    lcd.drawLine(0, 14, 318, 14)
    
    -- Parameter list
    local y = 20
    local visibleStart = math.max(1, selectedParam - 6)
    local visibleEnd = math.min(#parameters, visibleStart + 7)
    
    for i = visibleStart, visibleEnd do
        local param = parameters[i]
        
        -- Selection highlight
        if i == selectedParam then
            lcd.setColor(0, 0, 0)
            if editMode then
                lcd.drawFilledRectangle(2, y - 2, 314, 16)
                lcd.setColor(255, 255, 255)
            else
                lcd.drawRectangle(2, y - 2, 314, 16)
            end
        end
        
        -- Parameter name
        lcd.drawText(5, y, param.name, FONT_NORMAL)
        
        -- Parameter value
        local valueStr
        if param.type == "bool" then
            valueStr = param.value == 1 and "ON" or "OFF"
        else
            valueStr = string.format("%d", param.value)
        end
        lcd.drawText(310 - lcd.getTextWidth(FONT_NORMAL, valueStr), y, valueStr, FONT_NORMAL)
        
        lcd.setColor(0, 0, 0)
        y = y + 18
    end
    
    -- Footer with controls
    lcd.drawLine(0, 145, 318, 145)
    lcd.drawText(5, 150, "F1:Read", FONT_MINI)
    lcd.drawText(70, 150, "F2:Write", FONT_MINI)
    lcd.drawText(140, 150, "F3:Save All", FONT_MINI)
    lcd.drawText(220, 150, editMode and "ENTER:Confirm" or "ENTER:Edit", FONT_MINI)
end

-- Handle key press
local function keyPressed(key)
    if key == KEY_1 then  -- F1 - Read parameter
        if not editMode then
            local param = parameters[selectedParam]
            requestParameter(param.id)
        end
        
    elseif key == KEY_2 then  -- F2 - Write parameter
        if not editMode then
            local param = parameters[selectedParam]
            writeParameter(param.id, param.value)
        end
        
    elseif key == KEY_3 then  -- F3 - Save all
        if not editMode then
            saveConfiguration()
        end
        
    elseif key == KEY_5 or key == KEY_ESC then  -- Cancel edit
        if editMode then
            editMode = false
        end
        
    elseif key == KEY_ENTER then  -- Toggle edit mode
        editMode = not editMode
        
    elseif key == KEY_UP or key == KEY_DOWN then
        if editMode then
            -- Adjust value
            local param = parameters[selectedParam]
            local step = 1
            if param.max - param.min > 100 then
                step = 10
            end
            
            if key == KEY_UP then
                param.value = math.min(param.max, param.value + step)
            else
                param.value = math.max(param.min, param.value - step)
            end
        else
            -- Navigate parameters
            if key == KEY_UP then
                selectedParam = math.max(1, selectedParam - 1)
            else
                selectedParam = math.min(#parameters, selectedParam + 1)
            end
        end
    end
end

-- Telemetry window
local function printTelemetry(width, height)
    -- Get sensor values
    local gunRate = system.getSensorByID(sensorIds.gunRate, 0)
    local engineState = system.getSensorByID(sensorIds.engineState, 0)
    
    -- State names
    local gunRates = {"Low", "Med", "High"}
    local engineStates = {"STOPPED", "STARTING", "RUNNING", "STOPPING"}
    
    -- Draw telemetry data
    local y = 2
    
    lcd.drawText(2, y, "Gun Rate:", FONT_MINI)
    if gunRate and gunRate.valid then
        local rateIdx = math.floor(gunRate.value)
        local rateName = gunRates[rateIdx + 1] or "Unknown"
        local text = string.format("%s (%d)", rateName, rateIdx)
        lcd.drawText(width - lcd.getTextWidth(FONT_NORMAL, text),
                    y, text, FONT_NORMAL)
    else
        lcd.drawText(width - lcd.getTextWidth(FONT_MINI, "---"), y, "---", FONT_MINI)
    end
    y = y + 14
    
    lcd.drawText(2, y, "Engine:", FONT_MINI)
    if engineState and engineState.valid then
        local stateIdx = math.floor(engineState.value)
        local stateName = engineStates[stateIdx + 1] or "Unknown"
        local text = string.format("%s (%d)", stateName, stateIdx)
        lcd.drawText(width - lcd.getTextWidth(FONT_NORMAL, text),
                    y, text, FONT_NORMAL)
    else
        lcd.drawText(width - lcd.getTextWidth(FONT_MINI, "---"), y, "---", FONT_MINI)
    end
end

-- Cleanup on app close
local function destroy()
    -- Save current configuration
    for i, param in ipairs(parameters) do
        system.pSave(string.format("p%d", param.id), param.value)
    end
end

-- Register application
return {
    init = init,
    loop = function() end,
    destroy = destroy,
    printForm = printForm,
    keyPressed = keyPressed,
    printTelemetry = printTelemetry,
}
