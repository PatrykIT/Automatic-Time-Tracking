#ifndef QTDROPBOX_GLOBAL_H
#define QTDROPBOX_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(QTDROPBOX_LIBRARY)
#  define QTDROPBOXSHARED_EXPORT Q_DECL_EXPORT
#else
#  define QTDROPBOXSHARED_EXPORT Q_DECL_IMPORT
#endif

#ifndef QDROPBOX_HTTP_ERROR_CODES
#define QDROPBOX_HTTP_ERROR_CODES

/* TO DO: Make it enum, or encapsulate it somewhere in class */
/**
 * @brief HTTP Status Codes
 */
const qint32 QDROPBOX_ERROR_BAD_INPUT           = 400;
const qint32 QDROPBOX_ERROR_EXPIRED_TOKEN       = 401;
const qint32 QDROPBOX_ERROR_BAD_OAUTH_REQUEST   = 403;
const qint32 QDROPBOX_ERROR_FILE_NOT_FOUND      = 404;
const qint32 QDROPBOX_ERROR_WRONG_METHOD        = 405;
const qint32 QDROPBOX_ERROR_REQUEST_CAP         = 503;
const qint32 QDROPBOX_ERROR_USER_OVER_QUOTA     = 507;
#endif

#endif // QTDROPBOX_GLOBAL_H
