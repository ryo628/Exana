#ifndef _WRAPPER_H
#define _WRAPPER_H

typedef void* cache_sim_t;

#ifdef __cplusplus
extern "C"{
#endif

cache_sim_t CacheSimInit();
void CacheSimcheckAddr(cache_sim_t, unsigned long long int);
void CacheSimShowResult(cache_sim_t);

#ifdef __cplusplus
}
#endif

#endif