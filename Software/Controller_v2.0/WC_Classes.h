#ifndef WC_CLASSES_h
#define WC_CLASSES_h

//#define SDEBUG
/**
 * Класс работы с аналоговым входом
 */
class SInput {
   protected:
      uint8_t  Pin;
    
   private:
       uint16_t Samples; 
   public:
      uint16_t Value; 
      SInput(uint8_t _pin, uint16_t _samples = 1){
         Pin     = _pin;
         Samples = _samples;
         if( Samples == 0 )Samples = 1;         
      }
      uint16_t Get(){
         analogSetPinAttenuation(Pin, ADC_11db);
         uint32_t a = 0;
         for(int i=0; i<Samples; i++){
            a+=analogRead(Pin);
            delay(2);
         }
         a/=Samples;
         Value = a;
         return Value;
      }
};

/**
 * Класс работы с входом триггером
 * Сосотоянием управляет уровень на аналоговом входе
 */
class SInputTH: public SInput {
   private: 
       uint16_t Threshold;
   public:
       bool Stat;
       bool isChange;
       SInputTH(uint8_t _pin,uint16_t _th, uint16_t _samples):
        SInput(_pin,_samples){
           Threshold = _th;
           Stat = false;  
           isChange = false;  
        }
       bool GetStat(){
           bool _stat;
           if( Get() > Threshold )_stat = true;
           else _stat = false;
           if( _stat != Stat )isChange = true;
           Stat = _stat;    
       }
};

/**
 * Класс работы с NTC термометром подключенным к аналоговому порту
 */
class SInputNTC: public SInput {
  private:
     float referenceResistance;
     float nominalResistance;
     double nominalTemperature; // in Celsius. 
     double bValue;
     uint16_t adcResolution; 
     float referenceVoltage;
     float adcVoltage;
     float KR0;
     float KR1;
  public:
     float Temperature;
     SInputNTC( uint8_t _pin, float _ref, float _b, float _k0=0, float _k1=1):
        SInput(_pin, 64){
           referenceResistance = _ref;    
           nominalResistance   = 100000;
           nominalTemperature  = 25;
           bValue              = _b;
           adcResolution       = 4095;
           referenceVoltage    = 2.5;
           adcVoltage          = 3.3; 
           KR0                 = _k0;
           KR1                 = _k1;
        }
        float GetTemperature(){
           Get();
           float v0    = Value*adcVoltage*KR1/adcResolution+KR0;
           float r0    = referenceResistance*v0/(referenceVoltage-v0);
           float tk0   = 1.0/(1.0/(273.15+nominalTemperature)+log(r0/nominalResistance)/bValue);
           Temperature = tk0 - 273.15;
#ifdef SDEBUG           
           Serial.print("pin=");
           Serial.print(Pin);
           Serial.print(",A=");
           Serial.print(Value);
           Serial.print(",U=");
           Serial.print(v0);
           Serial.print(",R=");
           Serial.print(r0);
           Serial.print(",T=");
           Serial.println(Temperature);
#endif           
           return Temperature;
           
        }
};

class SOutput {
  private:
     uint8_t Pin;
     bool    Status;
  public:
     SOutput(uint8_t _pin, bool _stat = LOW){
         Pin = _pin;
         Status = _stat;
         pinMode(Pin,OUTPUT);
         digitalWrite(Pin,Status);        
     }
     bool Stat(){ 
         return Status; 
     }  
     void Set(bool _stat){
         Status = _stat;
         digitalWrite(Pin,Status);        
     }
     void Impulse(uint16_t _ms){
         Set(HIGH);
         delay(_ms);
         Set(LOW);
     }
};


#endif
