#include <Arduino.h>

constexpr unsigned long STOP_BIT_LOW_TIME  = 10000;

constexpr unsigned long START_BIT_HIGH_TIME = 3500;
constexpr unsigned long START_BIT_LOW_TIME  = 1700;

constexpr unsigned long BIT_HIGH_TIME  = 430;
constexpr unsigned long BIT_LOW_0_TIME = 440;
constexpr unsigned long BIT_LOW_1_TIME = 1300;

constexpr uint8_t PANASONIC_DATA_SIZE = 27;

enum StreamMode{
    AUTO,
    POWERFULL,
    QUIET
};

class PanasonicRemote{
    public:
        PanasonicRemote(byte pin_pwm, byte pin_led) : pin_pwm{pin_pwm}, pin_led{pin_led}, data{ new uint8_t[PANASONIC_DATA_SIZE] }{
            // Header
            data[0] = 0b00000010;
            data[1] = 0b00100000;
            data[2] = 0b11100000;
            data[3] = 0b00000100;
            data[4] = 0b00000000;
            data[5] = 0b00000000;
            data[6] = 0b00000000;
            data[7] = 0b00000110;

            // Body
            data[8] = 0b00000010;
            data[9] = 0b00100000;
            data[10] = 0b11100000;
            data[11] = 0b00000100;
            data[12] = 0b00000000;
            data[13] = 0b00001000;
            data[14] = 0b00110010;
            data[15] = 0b10000000;
            data[16] = 0b10101111;
            data[17] = 0b00000000;
            data[18] = 0b00000000;
            data[29] = 0b00001110;
            data[20] = 0b11100000;
            data[21] = 0b00000000;
            data[22] = 0b00000000;
            data[23] = 0b10001001;
            data[24] = 0b00000000;
            data[25] = 0b00000000;
            data[26] = 0b11100110;
        }

        void init(){
            pinMode(pin_pwm, OUTPUT);
            analogWriteRange(1024);
            analogWriteFreq(38000);
            analogWrite(pin_pwm, 512);
            pinMode(pin_led, OUTPUT);
            digitalWrite(pin_led, LOW);
        }

        PanasonicRemote& turnOn(){
            setBit(13, 0);
            compute_checksum(); 
            return *this;
        }

        PanasonicRemote& turnOff(){
            setStreamMode(StreamMode::AUTO);    // Même comportement que la télécommand d'origine
            clearBit(13, 0);
            compute_checksum();
            return *this;
        }

        PanasonicRemote& setStreamMode(StreamMode mode){

            switch(mode){
                case StreamMode::POWERFULL:
                    clearBit(21, 5);
                    setBit(21, 0);
                    break;
                
                case StreamMode::QUIET:
                    clearBit(21, 0);
                    setBit(21, 5);
                    break;
                
                default:
                case StreamMode::AUTO:
                    clearBit(21, 5);
                    clearBit(21, 0);
                    break;
            }
            
            compute_checksum();
            return *this;
        }

        PanasonicRemote& setTemperature(uint8_t temp, bool half){

            if( 16 <= temp && temp <= 30){

                data[14] &= 0b11000001;
                data[14] |= temp << 1;

                if( half && temp < 30){
                    setBit(14, 0);
                    compute_checksum();
                }
                else{
                    clearBit(14, 0);
                    compute_checksum();
                }
            }
            return *this;
        }

        void send(){
            
            start_bit();

            // Header
            for( unsigned octet = 0; octet < 8; ++octet ){
                for( unsigned bit = 0; bit < 8; bit++ ){
                    set_high();
                    delayMicroseconds(BIT_HIGH_TIME);
                    set_low();

                    if( (data[octet] & (1 << bit)) > 0 )
                        delayMicroseconds(BIT_LOW_1_TIME);
                    else
                        delayMicroseconds(BIT_LOW_0_TIME);
                }
            }

            stop_bit();
            start_bit();

            // Body
            for( unsigned octet = 8; octet < PANASONIC_DATA_SIZE; ++octet ){
                for( unsigned bit = 0; bit < 8; bit++ ){
                    set_high();
                    delayMicroseconds(BIT_HIGH_TIME);
                    set_low();

                    if( (data[octet] & (1 << bit)) > 0 )
                        delayMicroseconds(BIT_LOW_1_TIME);
                    else
                        delayMicroseconds(BIT_LOW_0_TIME);
                }
            }

            stop_bit();
        }

    private:
        byte pin_pwm;
        byte pin_led;
        uint8_t* data;

        inline void set_high(){
            digitalWrite(pin_led, HIGH);
        }

        inline void set_low(){
            digitalWrite(pin_led, LOW);
        }

        void setBit(uint8_t byte, uint8_t bit){
            if( byte >= PANASONIC_DATA_SIZE || bit >= 8){
                return;
            }

            data[byte] |= (0x01 << bit);
        }

        void clearBit(uint8_t byte, uint8_t bit){
            if( byte >= PANASONIC_DATA_SIZE || bit >= 8){
                return;
            }

            data[byte] &= ~(0x01 << bit);
        }

        void compute_checksum(){
            uint32_t sum = 0;

            for(unsigned i = 8; i < (PANASONIC_DATA_SIZE - 1); ++i){
                sum += data[i];
            }

            data[PANASONIC_DATA_SIZE - 1] = (sum % 256);
        }

        void start_bit(){
            set_high();
            delayMicroseconds(START_BIT_HIGH_TIME);
            set_low();
            delayMicroseconds(START_BIT_LOW_TIME);
        }

        void stop_bit(){
            set_high();
            delayMicroseconds(BIT_HIGH_TIME);
            set_low();
            delayMicroseconds(STOP_BIT_LOW_TIME);
        }
};
