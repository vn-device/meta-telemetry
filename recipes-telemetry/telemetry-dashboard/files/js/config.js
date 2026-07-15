const piPinout = [
    { pin: 1, name: '3.3V Power', type: 'power3v3' },  { pin: 2, name: '5V Power', type: 'power5v' },
    { pin: 3, name: 'I2C1 Data (SDA) / I/O', type: 'gpio' },   { pin: 4, name: '5V Power', type: 'power5v' },
    { pin: 5, name: 'I2C1 Clock (SCL) / I/O', type: 'gpio' },   { pin: 6, name: 'Ground', type: 'gnd' },
    { pin: 7, name: 'General I/O / Clock', type: 'gpio' },{ pin: 8, name: 'UART0 Transmit (TXD) / I/O', type: 'gpio', highlight: true },
    { pin: 9, name: 'Ground', type: 'gnd' },          { pin: 10, name: 'UART0 Receive (RXD) / I/O', type: 'gpio' },
    { pin: 11, name: 'General I/O', type: 'gpio' },       { pin: 12, name: 'PWM0 / General I/O', type: 'gpio' },
    { pin: 13, name: 'General I/O', type: 'gpio' },       { pin: 14, name: 'Ground', type: 'gnd' },
    { pin: 15, name: 'General I/O', type: 'gpio' },       { pin: 16, name: 'General I/O', type: 'gpio' },
    { pin: 17, name: '3.3V Power', type: 'power3v3' }, { pin: 18, name: 'General I/O', type: 'gpio' },
    { pin: 19, name: 'SPI0 MOSI / I/O', type: 'gpio' },{ pin: 20, name: 'Ground', type: 'gnd' },
    { pin: 21, name: 'SPI0 MISO / I/O', type: 'gpio' }, { pin: 22, name: 'General I/O', type: 'gpio' },
    { pin: 23, name: 'SPI0 Clock (SCLK) / I/O', type: 'gpio' },{ pin: 24, name: 'SPI0 Chip Enable 0 / I/O', type: 'gpio' },
    { pin: 25, name: 'Ground', type: 'gnd' },         { pin: 26, name: 'SPI0 Chip Enable 1 / I/O', type: 'gpio' },
    { pin: 27, name: 'Reserved (HAT Data)', type: 'reserved' },{ pin: 28, name: 'Reserved (HAT Clock)', type: 'reserved' },
    { pin: 29, name: 'General I/O', type: 'gpio' },        { pin: 30, name: 'Ground', type: 'gnd' },
    { pin: 31, name: 'General I/O', type: 'gpio' },        { pin: 32, name: 'PWM0 / General I/O', type: 'gpio' },
    { pin: 33, name: 'PWM1 / General I/O', type: 'gpio' },{ pin: 34, name: 'Ground', type: 'gnd' },
    { pin: 35, name: 'SPI1 MISO / I/O', type: 'gpio' },{ pin: 36, name: 'General I/O', type: 'gpio' },
    { pin: 37, name: 'General I/O', type: 'gpio' },       { pin: 38, name: 'SPI1 MOSI / I/O', type: 'gpio' },
    { pin: 39, name: 'Ground', type: 'gnd' },         { pin: 40, name: 'SPI1 Clock (SCLK) / I/O', type: 'gpio' }
];

const frontendGpioState = {};

const cameraState = {
    isStreaming: false,
    isRecording: false,
    resolution: "1920x1080"
};