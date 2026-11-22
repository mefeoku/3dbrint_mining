
#ifndef MINING_API_H
#define MINING_API_H

// Mining
#define MAX_NONCE_STEP  5000000U
#define MAX_NONCE       25000000U
#define TARGET_NONCE    471136297U
#define DEFAULT_DIFFICULTY  0.00015
#define KEEPALIVE_TIME_ms       30000
#define POOLINACTIVITY_TIME_ms  60000

#if !defined(CONFIG_IDF_TARGET_ESP32C3) && (defined(CONFIG_IDF_TARGET_ESP32S2) || defined(CONFIG_IDF_TARGET_ESP32S3))
#define HARDWARE_SHA265
#endif

#define TARGET_BUFFER_SIZE 64

extern uint32_t pool_ping_ms;
extern volatile bool is_mining_paused_flag;
extern volatile bool g_stratumReconnectNeeded;
extern uint32_t templates;
extern uint32_t Mhashes;
extern uint32_t totalKHashes;
extern uint64_t upTime;
extern volatile uint32_t shares;
extern volatile uint32_t valids;
extern double best_diff;

void runMonitor(void *name);

void runStratumWorker(void *name);
void runMiner(void *name);

void minerWorkerSw(void * task_id);
void minerWorkerHw(void * task_id);

String printLocalTime(void);

void resetStat();

typedef struct{
  uint8_t bytearray_target[32];
  uint8_t bytearray_pooltarget[32];
  uint8_t merkle_result[32];
  uint8_t bytearray_blockheader[128];
} miner_data;


#endif // UTILS_API_H