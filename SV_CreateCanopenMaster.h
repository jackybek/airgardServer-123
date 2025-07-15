#ifndef SV_CREATECANOPENMASTER_H
  #define SV_CREATECANOPENMASTER_H

  #ifdef __cplusplus
    extern "C" {
  #endif

  void *initialiseCanopen(void *arg);
  int CreateCanOpenDeviceFromEdsDcf(co_dev_t **);

  #ifdef __cplusplus
    }
  #endif

#endif // CPP_FUNCTIONS_H
