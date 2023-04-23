#ifndef H_ACQUIRE_DCAM_PRELUDE_V0
#define H_ACQUIRE_DCAM_PRELUDE_V0

#define L (aq_logger)
#define LOG(...) L(0, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define ERR(...) L(1, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)

// #define TRACE(...) LOG(__VA_ARGS__)
#define TRACE(...)

#define DISFAIL(ecode) ((int)(ecode) < 0)

#define EXPECT_INNER(eval, test, action, logger, ...)                          \
    do {                                                                       \
        eval;                                                                  \
        if (!(test)) {                                                         \
            logger(__VA_ARGS__);                                               \
            action;                                                            \
        }                                                                      \
    } while (0)
#define EXPECT(e, ...) EXPECT_INNER(, e, goto Error, ERR, __VA_ARGS__)
#define CHECK(e) EXPECT(e, "Expression was false:\n\t%s\n", #e)
#define WARN(e) EXPECT_INNER(, e, , LOG, "Expression was false:\n\t%s\n", #e)
#define DCAM_INNER(e, logger, action)                                          \
    EXPECT_INNER(DCAMERR result_ = (e),                                        \
                 !DISFAIL(result_),                                            \
                 action,                                                       \
                 logger,                                                       \
                 "Expression failed:\n\t%s\n\t%s\n",                           \
                 #e,                                                           \
                 dcam_error_to_string(result_))

#define DCAM(e) DCAM_INNER(e, ERR, goto Error)
#define DWRN(e) DCAM_INNER(e, LOG, )

#endif // H_ACQUIRE_DCAM_PRELUDE_V0
