#ifndef IGNUM02_AUTH_DECODER_H
#define IGNUM02_AUTH_DECODER_H

#include <Arduino.h>
#include "PARAMS.h"

class IGNUM{
      public:

      String GetChallenge();
      
      bool GetRxValid();
      String GetRxRootKey();
      String GetRxCommand();
      String GetRxCondit1();
      String GetRxCondit2();
      String GetRxCondit3();
      void EndRxCommand(); //CLEAN LOADED COMMAND

      void SustainLoop(); //LOOP
      String begin(String reboot); // 1
      void reload(); // 2
      bool InputPlainCode(String inputPack); // 3
      bool InputCyphercode(String CypherCode); // Crypt Input;
      String GenIgnumCommand(String Chlg, String Command); // Crypt Output;  

      void setChaosFactor(String input);
};


#endif
