#ifndef LOGGING_H_
#define LOGGING_H_

#include "rdk_debug.h"

#define LOG_NMGR "LOG.RDK.NETSRVMGR"

/**
 * @brief NETSRVMGR_LOG_TYPE
 * @{
 */
#define LOG_ERR(fmt, ...)   RDK_LOG(RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] " fmt "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)  RDK_LOG(RDK_LOG_INFO, LOG_NMGR, "[%s:%d] " fmt "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  RDK_LOG(RDK_LOG_WARN, LOG_NMGR, "[%s:%d] " fmt "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define LOG_DBG(fmt, ...)   RDK_LOG(RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d] " fmt "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define LOG_TRACE(fmt, ...) RDK_LOG(RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] " fmt "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define LOG_FATAL(fmt, ...) RDK_LOG(RDK_LOG_FATAL, LOG_NMGR, "[%s:%d] " fmt "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__)

/** @} */  //END OF NET_LOG_TYPE

class EntryExitLogger
{
    const char* func;
    const int line;
public:
    EntryExitLogger (const char* func, const int line) : func (func), line (line)
    {
        RDK_LOG(RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Entry\n", func, line);
    }
    ~EntryExitLogger ()
    {
        RDK_LOG(RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit \n", func, line);
    }
};

#define LOG_ENTRY_EXIT EntryExitLogger entry_exit_logger (__FUNCTION__, __LINE__)

#endif /* LOGGING_H_ */
