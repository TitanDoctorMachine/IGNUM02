# IGNUM02
light version of ignum01








*FOR CONFIGURING USER, CREATE THE FILE 'PARAMS.h':*

        
    #ifndef PARAMS_H
    #define PARAMS_H
    #include <Arduino.h>

    class PARAMS{
        public:

        String GetUser() {return "yourusercomesheretestetstets";}
        const char * GetMASTERWIFISSID() {return "DOYOUWANNAKNOW?";}
        const char * GetMASTERWIFIPSWD() {return "000000000";}
        String GetMASTERSERVERIP() {return "192.168.XXX.XXX";}
        
    };
    #endif
