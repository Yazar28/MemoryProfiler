#pragma once
#ifdef MT_DEBUG
  #include <iostream>
  #define MT_LOG(expr)   do { std::cout << expr; } while(0)
  #define MT_LOGLN(expr) do { std::cout << expr << '\n'; } while(0)
#else
  #define MT_LOG(expr)   do {} while(0)
  #define MT_LOGLN(expr) do {} while(0)
#endif
