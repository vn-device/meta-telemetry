const piPinout = [
    { pin: 1, name: '3V3 Power', type: 'power3v3' },  { pin: 2, name: '5V Power', type: 'power5v' },
    { pin: 3, name: 'GPIO 2 (SDA)', type: 'gpio' },   { pin: 4, name: '5V Power', type: 'power5v' },
    { pin: 5, name: 'GPIO 3 (SCL)', type: 'gpio' },   { pin: 6, name: 'Ground', type: 'gnd' },
    { pin: 7, name: 'GPIO 4 (GPCLK0)', type: 'gpio' },{ pin: 8, name: 'GPIO 14 (TXD)', type: 'gpio', highlight: true },
    { pin: 9, name: 'Ground', type: 'gnd' },          { pin: 10, name: 'GPIO 15 (RXD)', type: 'gpio' },
    { pin: 11, name: 'GPIO 17', type: 'gpio' },       { pin: 12, name: 'GPIO 18 (PCM_CLK)', type: 'gpio' },
    { pin: 13, name: 'GPIO 27', type: 'gpio' },       { pin: 14, name: 'Ground', type: 'gnd' },
    { pin: 15, name: 'GPIO 22', type: 'gpio' },       { pin: 16, name: 'GPIO 23', type: 'gpio' },
    { pin: 17, name: '3V3 Power', type: 'power3v3' }, { pin: 18, name: 'GPIO 24', type: 'gpio' },
    { pin: 19, name: 'GPIO 10 (SPI0 MOSI)', type: 'gpio' },{ pin: 20, name: 'Ground', type: 'gnd' },
    { pin: 21, name: 'GPIO 9 (SPI0 MISO)', type: 'gpio' }, { pin: 22, name: 'GPIO 25', type: 'gpio' },
    { pin: 23, name: 'GPIO 11 (SPI0 SCLK)', type: 'gpio' },{ pin: 24, name: 'GPIO 8 (SPI0 CE0)', type: 'gpio' },
    { pin: 25, name: 'Ground', type: 'gnd' },         { pin: 26, name: 'GPIO 7 (SPI0 CE1)', type: 'gpio' },
    { pin: 27, name: 'GPIO 0 (ID_SD)', type: 'reserved' },{ pin: 28, name: 'GPIO 1 (ID_SC)', type: 'reserved' },
    { pin: 29, name: 'GPIO 5', type: 'gpio' },        { pin: 30, name: 'Ground', type: 'gnd' },
    { pin: 31, name: 'GPIO 6', type: 'gpio' },        { pin: 32, name: 'GPIO 12 (PWM0)', type: 'gpio' },
    { pin: 33, name: 'GPIO 13 (PWM1)', type: 'gpio' },{ pin: 34, name: 'Ground', type: 'gnd' },
    { pin: 35, name: 'GPIO 19 (SPI1 MISO)', type: 'gpio' },{ pin: 36, name: 'GPIO 16', type: 'gpio' },
    { pin: 37, name: 'GPIO 26', type: 'gpio' },       { pin: 38, name: 'GPIO 20 (SPI1 MOSI)', type: 'gpio' },
    { pin: 39, name: 'Ground', type: 'gnd' },         { pin: 40, name: 'GPIO 21 (SPI1 SCLK)', type: 'gpio' }
];

const frontendGpioState = {};
