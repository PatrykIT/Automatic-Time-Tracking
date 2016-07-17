#include "qdropbox.h"
#include <QNetworkReply>
#include <qnetworkreply.h>

#include "assert.h"

//TO DO: Delegating constructors.
QDropbox::QDropbox(QObject *parent) :
    QObject(parent),
    conManager(this)
{
    errorState = QDropbox::NoError;
    errorText  = "";
    setApiVersion("1.0");
    setApiUrl("api.dropbox.com");
    setAuthMethod(QDropbox::Plaintext);

    oauthToken = "";
    oauthTokenSecret = "";

    lastreply = 0;

    connect(&conManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(networkReplyFinished(QNetworkReply*)));
    connect(this, SIGNAL(errorOccured(QDropbox::Error)),
            this, SLOT(Output_Error(QDropbox::Error)));

    // needed for nonce generation
    qsrand(QDateTime::currentMSecsSinceEpoch());

    _evLoop = NULL;
	_saveFinishedRequests = false;
}

QDropbox::QDropbox(QString key, QString sharedSecret, OAuthMethod method, QString url, QObject *parent) :
    QObject(parent),
    conManager(this)
{
    errorState      = QDropbox::NoError;
    errorText       = "";
    setKey(key);
    setSharedSecret(sharedSecret);
    setAuthMethod(method);
    setApiVersion("1.0");
    setApiUrl(url);

    oauthToken = "";
    oauthTokenSecret = "";

    lastreply = 0;

    connect(&conManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(networkReplyFinished(QNetworkReply*)));

    // needed for nonce generation
    qsrand(QDateTime::currentMSecsSinceEpoch());

    _evLoop = NULL;
	_saveFinishedRequests = false;
}

void QDropbox::Output_Error(QDropbox::Error errorcode)
{
    qDebug() << "Error code: " << errorcode;
    qDebug() << "Error text: " << errorText;
}

QDropbox::Error QDropbox::error()
{
    return errorState;
}

QString QDropbox::errorString()
{
    return errorText;
}

void QDropbox::setApiUrl(QString url)
{
    /* %1 will be replaced by the 'url' */
    apiurl.setUrl(QString("//%1").arg(url));
    //apiurl.setUrl(QString("//") + url);
    prepareApiUrl();

    return;
}

QString QDropbox::apiUrl()
{
    return apiurl.toString();
}

void QDropbox::setAuthMethod(OAuthMethod m)
{
    oauthMethod = m;
    prepareApiUrl();
    return;
}

QDropbox::OAuthMethod QDropbox::authMethod()
{
    return oauthMethod;
}

void QDropbox::setApiVersion(QString apiversion)
{
    if(apiversion.compare("1.0") != 0) //0 means strings are equal.
    {
        errorState = QDropbox::VersionNotSupported;
        errorText  = "Only version 1.0 is supported.";

        emit errorOccured(QDropbox::VersionNotSupported);
        return;
    }

    _version = apiversion;
    return;
}

void QDropbox::requestFinished(int nr, QNetworkReply *rply)
{
    rply->deleteLater();

#ifdef QTDROPBOX_DEBUG
    int resp_bytes = rply->bytesAvailable();
#endif

    QByteArray buff = rply->readAll();
    QString response = QString(buff);

#ifdef QTDROPBOX_DEBUG
    qDebug() << "request " << nr << "finished.";
    qDebug() << "request was: " << rply->url().toString();
    qDebug() << "response: " << resp_bytes << "bytes";
    qDebug() << "status code: " << rply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toString();
    qDebug() << "== begin response ==" << endl << response << endl << "== end response ==";
    qDebug() << "req#" << nr << " is of type " << requestMap[nr].type;
#endif

    // drop box error handling based on return codes
    switch(rply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt())
    {
    case QDROPBOX_ERROR_BAD_INPUT:
        errorState = QDropbox::BadInput;
        errorText  = "";
        emit errorOccured(errorState);
        checkReleaseEventLoop(nr);
        return;
    case QDROPBOX_ERROR_EXPIRED_TOKEN:
        errorState = QDropbox::TokenExpired;
        errorText  = "";
        emit tokenExpired();
        checkReleaseEventLoop(nr);
        return;
    case QDROPBOX_ERROR_BAD_OAUTH_REQUEST:
        errorState = QDropbox::BadOAuthRequest;
        errorText  = "";
        emit errorOccured(errorState);
        checkReleaseEventLoop(nr);
        return;
    case QDROPBOX_ERROR_FILE_NOT_FOUND:
        emit fileNotFound();
        checkReleaseEventLoop(nr);
        return;
    case QDROPBOX_ERROR_WRONG_METHOD:
        errorState = QDropbox::WrongHttpMethod;
        errorText  = "";
        emit errorOccured(errorState);
        checkReleaseEventLoop(nr);
        return;
    case QDROPBOX_ERROR_REQUEST_CAP:
        errorState = QDropbox::MaxRequestsExceeded;
        errorText = "";
        emit errorOccured(errorState);
        checkReleaseEventLoop(nr);
        return;
    case QDROPBOX_ERROR_USER_OVER_QUOTA:
        errorState = QDropbox::UserOverQuota;
        errorText = "";
        emit errorOccured(errorState);
        checkReleaseEventLoop(nr);
        return;
    default:
        break;
    }

    if(rply->error() != QNetworkReply::NoError)
    {
        errorState = QDropbox::CommunicationError;
        errorText  = QString("%1 - %2").arg(rply->error()).arg(rply->errorString());

        emit errorOccured(errorState);
		checkReleaseEventLoop(nr);
        return;
    }

    // ignore connection requests
    if(requestMap[nr].type == QDROPBOX_REQ_CONNECT)
    {
        #ifdef QTDROPBOX_DEBUG
                qDebug() << "- answer to connection request ignored" << endl;
        #endif
		removeRequestFromMap(nr);
        return;
    }

    bool delayed_finish = false;
    int delayed_nr;

    if(rply->attribute(QNetworkRequest::HttpStatusCodeAttribute) == 302) //TO DO: Check if this could be an enum, or define somewhere 302 = xxx
    {
        // redirection handling
        QUrl newlocation(rply->header(QNetworkRequest::LocationHeader).toString(), QUrl::StrictMode);

        #ifdef QTDROPBOX_DEBUG
                    qDebug() << "Redirection received. New url: " << newlocation.toString();
        #endif
        int oldnr = nr;

        nr = sendRequest(newlocation, requestMap[nr].method, 0, requestMap[nr].host);
        requestMap[nr].type = QDROPBOX_REQ_REDIREC;
        requestMap[nr].linked = oldnr;
        return;
    }
    else
    {
        if(requestMap[nr].type == QDROPBOX_REQ_REDIREC)
        {
            // change values if this is the answer to a redirect
            qdropbox_request redir = requestMap[nr];
            qdropbox_request orig  = requestMap[redir.linked];
            requestMap[nr] = orig;
            //requestMap[nr] = requestMap[redir.linked];
			removeRequestFromMap(nr);
            nr = redir.linked;
        }

        // standard handling depending on message type
        switch(requestMap[nr].type)
        {
        case QDROPBOX_REQ_CONNECT:
            // was only a connect request - so drop it
            break;
        case QDROPBOX_REQ_RQTOKEN:
            // requested a token
            responseTokenRequest(response);
            break;
        case QDROPBOX_REQ_RQBTOKN:
            responseBlockedTokenRequest(response);
            break;
        case QDROPBOX_REQ_AULOGIN:
            delayed_nr = responseDropboxLogin(response, nr);
            delayed_finish = true;
            break;
        case QDROPBOX_REQ_ACCTOKN:
            responseAccessToken(response);
            break;
        case QDROPBOX_REQ_METADAT:
            parseMetadata(response);
            break;
        case QDROPBOX_REQ_BMETADA:
            parseBlockingMetadata(response);
			break;
        case QDROPBOX_REQ_BACCTOK:
            responseBlockingAccessToken(response);
            break;
        case QDROPBOX_REQ_ACCINFO:
            parseAccountInfo(response);
            break;
        case QDROPBOX_REQ_BACCINF:
            parseBlockingAccountInfo(response);
            break;
        case QDROPBOX_REQ_SHRDLNK:
            parseSharedLink(response);
            break;
        case QDROPBOX_REQ_BSHRDLN:
            parseBlockingSharedLink(response);
            break;
		case QDROPBOX_REQ_REVISIO:
			parseRevisions(response);
			break;
		case QDROPBOX_REQ_BREVISI:
			parseBlockingRevisions(response);
			break;
        case QDROPBOX_REQ_DELTA:
            parseDelta(response);
            break;
        case QDROPBOX_REQ_BDELTA:
            parseBlockingDelta(response);
            break;
        default:
            errorState  = QDropbox::ResponseToUnknownRequest;
            errorText   = "Received a response to an unknown request";
            emit errorOccured(errorState);
            break;
        }
    }

    if(delayed_finish)
        delayMap[delayed_nr] = nr;
    else
    {
        if(delayMap[nr])
        {
            int drq = delayMap[nr];
            while(drq != 0)
            {
                emit operationFinished(delayMap[drq]);
                delayMap.remove(drq);
                drq = delayMap[drq];
            }
        }

		removeRequestFromMap(nr);
        emit operationFinished(nr);
    }

    return;
}

void QDropbox::networkReplyFinished(QNetworkReply *rply)
{
    int reqnr = replynrMap[rply];
    requestFinished(reqnr, rply);
    rply->deleteLater(); // release memory
}


QString QDropbox::hmacsha1(QString baseString, QString key)
{
    int blockSize = 64; // HMAC-::hmacsha1SHA-1 block size, defined in SHA-1 standard
    if (key.length() > blockSize) { // if key is longer than block size (64), reduce key length with SHA-1 compression
        key = QCryptographicHash::hash(key.toLatin1(), QCryptographicHash::Sha1);
    }

    QByteArray innerPadding(blockSize, char(0x36)); // initialize inner padding with char "6"
    QByteArray outerPadding(blockSize, char(0x5c)); // initialize outer padding with char "\"
    // ascii characters 0x36 ("6") and 0x5c ("\") are selected because they have large
    // Hamming distance (http://en.wikipedia.org/wiki/Hamming_distance)

    for (int i = 0; i < key.length(); i++) {
        innerPadding[i] = innerPadding[i] ^ key.toLatin1().at(i); // XOR operation between every byte in key and innerpadding, of key length
        outerPadding[i] = outerPadding[i] ^ key.toLatin1().at(i); // XOR operation between every byte in key and outerpadding, of key length
    }

    // result = hash ( outerPadding CONCAT hash ( innerPadding CONCAT baseString ) ).toBase64
    QByteArray total = outerPadding;
    QByteArray part = innerPadding;
    part.append(baseString.toLatin1());
    total.append(QCryptographicHash::hash(part, QCryptographicHash::Sha1));
    QByteArray hashed = QCryptographicHash::hash(total, QCryptographicHash::Sha1);
    return hashed.toBase64();
}

QString QDropbox::generateNonce(qint32 length)
{
    QString clng = "";
    for(int i = 0; i < length; ++i)
        clng += QString::number(int( qrand() / (RAND_MAX + 1.0) * (16 + 1 - 0) + 0 ), 16).toUpper();
    return clng;
}

QString QDropbox::oAuthSign(QUrl base, QString method)
{
    if(oauthMethod == QDropbox::Plaintext)
    {
    #ifdef QTDROPBOX_DEBUG
            qDebug() << "oauthMethod = Plaintext";
    #endif
        return QString("%1&%2").arg(_appSharedSecret).arg(oauthTokenSecret);
    }

    QString param   = base.toString(QUrl::RemoveAuthority|QUrl::RemovePath|QUrl::RemoveScheme).mid(1);
    param = QUrl::toPercentEncoding(param);
    QString requrl  = base.toString(QUrl::RemoveQuery);
    requrl = QUrl::toPercentEncoding(requrl);

    QString baseurl = method + "&" + requrl + "&" + param;
    QString key     = QString("%1&%2").arg(_appSharedSecret).arg(oauthTokenSecret);
    #ifdef QTDROPBOX_DEBUG
        qDebug() << "param = " << param << endl << "requrl = " << requrl;
        qDebug() << "baseurl = " << baseurl << " endbase";
        qDebug() << "key = " << key << " endkey";
    #endif

    QString signature = "";
    if(oauthMethod == QDropbox::HMACSHA1)
        signature = hmacsha1(baseurl.toUtf8(), key.toUtf8());
    else
    {
        errorState = QDropbox::UnknownAuthMethod;
        errorText  = QString("Authentication method %1 is unknown").arg(oauthMethod);

        emit errorOccured(errorState);
        return "";
    }

    #ifdef QTDROPBOX_DEBUG
        qDebug() << "key = " << key << endl;
        qDebug() << "signature = " << signature << "(base64 = " << QByteArray(signature.toUtf8()).toBase64();
    #endif

    return signature.toUtf8();
}

void QDropbox::prepareApiUrl()
{
    //if(oauthMethod == QDropbox::Plaintext)
    apiurl.setScheme("https");
    //else
    //  apiurl.setScheme("http");
}

int QDropbox::sendRequest(QUrl request, QString type, QByteArray postdata, QString host)
{
    /* If host is not provided (empty), retrieve it from apiurl member  */
    if(host.trimmed().compare("") == 0) //trimmed removes whitespaces from start and end of string (internal whitespaces stay the same).
        host = apiurl.toString(QUrl::RemoveScheme).mid(2);

    #ifdef QTDROPBOX_DEBUG
        qDebug() << "sendRequest() host = " << host << endl;
    #endif

    /* TO DO: Check if this variable is needed. */
    QString req_str = request.toString(QUrl::RemoveAuthority|QUrl::RemoveScheme);
    if(!req_str.startsWith("/"))
        req_str = QString("/%1").arg(req_str);

    QNetworkRequest rq(request);
    QNetworkReply *rply;

    if(type.compare("GET") == 0)
    {
        rply = conManager.get(rq);
    }
    else if(type.compare("POST") == 0)
    {
        assert(postdata.isEmpty() == false);

        rq.setHeader( QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded" );
        rply = conManager.post(rq, postdata);
    }
    else
    {
        errorState = QDropbox::UnknownQueryMethod;
        errorText  = "The provided query method is unknown.";

        emit errorOccured(errorState);
        return -1;
    }

    replynrMap[rply] = ++lastreply;

    requestMap[lastreply].method = type;
    requestMap[lastreply].host   = host;

    #ifdef QTDROPBOX_DEBUG
        qDebug() << "sendRequest() -> request #" << lastreply << " sent." << endl;
    #endif

    emit operationStarted(lastreply); // fire signal for operation start
    return lastreply;
}

void QDropbox::responseTokenRequest(QString response)
{
    parseToken(response);
    emit requestTokenFinished(oauthToken, oauthTokenSecret);
    return;
}

int QDropbox::responseDropboxLogin(QString response, int reqnr)
{
    Q_UNUSED(reqnr);

    // extract login form
    QDomDocument xml;
    QString err;
    int lnr, cnr;
    if(!xml.setContent(response, false, &err, &lnr, &cnr))
    {
#ifdef QTDROPBOX_DEBUG
        qDebug() << "invalid xml (" << lnr << "," << cnr << "): " << err << "dump:" << endl;
        qDebug() << xml.toString() << endl;
#endif
        return 0;
    }
    return 0;
}

void QDropbox::responseAccessToken(QString response)
{
    parseToken(response);
    emit accessTokenFinished(oauthToken, oauthTokenSecret);
    return;
}

QString QDropbox::signatureMethodString()
{
    switch(oauthMethod)
    {
    case QDropbox::Plaintext:
        return "PLAINTEXT";
    case QDropbox::HMACSHA1:
        return "HMAC-SHA1";
    default:
        errorState = QDropbox::UnknownAuthMethod;
        errorText  = QString("Authentication method %1 is unknown").arg(oauthMethod);
        emit errorOccured(errorState);
        return "";
    }
}

void QDropbox::parseToken(QString response)
{
	clearError();

    QStringList split = response.split("&");
    if(split.size() < 2)
    {
        errorState = QDropbox::APIError;
        errorText  = "The Dropbox API did not respond as expected.";

        emit errorOccured(errorState);
        return;
    }

    if(!split.at(0).startsWith("oauth_token_secret") ||
            !split.at(1).startsWith("oauth_token"))
    {
        errorState = QDropbox::APIError;
        errorText  = "The Dropbox API did not respond as expected.";

        emit errorOccured(errorState);
        return;
    }

    QStringList tokenSecretList = split.at(0).split("=");
    oauthTokenSecret = tokenSecretList.at(1);
    QStringList tokenList = split.at(1).split("=");
    oauthToken = tokenList.at(1);

    #ifdef QTDROPBOX_DEBUG
        qDebug() << "token = " << oauthToken << endl << "token_secret = " << oauthTokenSecret << endl;
    #endif

    emit tokenChanged(oauthToken, oauthTokenSecret);
    return;
}

void QDropbox::parseAccountInfo(QString response)
{
    #ifdef QTDROPBOX_DEBUG
        qDebug() << "== account info ==" << response << "== account info end ==";
    #endif

    QDropboxJson json;
    json.parseString(response);
    _tempJson.parseString(response);

    if(!json.isValid())
    {
        errorState = QDropbox::APIError;
        errorText  = "Dropbox API did not send correct answer for account information.";

        emit errorOccured(errorState);
        return;
    }

    emit accountInfoReceived(response);
}

void QDropbox::parseSharedLink(QString response)
{
    #ifdef QTDROPBOX_DEBUG
        qDebug() << "== shared link ==" << response << "== shared link end ==";
    #endif

    _tempJson.parseString(response);
    if(!_tempJson.isValid())
    {
        errorState = QDropbox::APIError;
        errorText  = "Dropbox API did not send correct answer for file/directory shared link.";

        emit errorOccured(errorState);
        stopEventLoop();
        return;
    }
    emit sharedLinkReceived(response);
}

void QDropbox::parseMetadata(QString response)
{
    #ifdef QTDROPBOX_DEBUG
        qDebug() << "== metadata ==" << response << "== metadata end ==";
    #endif

    QDropboxJson json;
    json.parseString(response);
    _tempJson.parseString(response);
    if(!json.isValid())
    {
        errorState = QDropbox::APIError;
        errorText  = "Dropbox API did not send correct answer for file/directory metadata.";

        emit errorOccured(errorState);
        stopEventLoop();
        return;
    }

    emit metadataReceived(response);
}

void QDropbox::parseDelta(QString response)
{
    #ifdef QTDROPBOX_DEBUG
        qDebug() << "== metadata ==" << response << "== metadata end ==";
    #endif

    QDropboxJson json;
    json.parseString(response);
    _tempJson.parseString(response);

    if(!json.isValid())
    {
        errorState = QDropbox::APIError;
        errorText  = "Dropbox API did not send correct answer for delta.";

        emit errorOccured(errorState);
        stopEventLoop();
        return;
    }

    emit deltaReceived(response);
}

void QDropbox::setKey(QString key)
{
    #ifdef QTDROPBOX_DEBUG
        qDebug() << "appKey = " << key;
    #endif
    _appKey = key;
}

QString QDropbox::key() const
{
    return _appKey;
}

void QDropbox::setSharedSecret(QString sharedSecret)
{
    #ifdef QTDROPBOX_DEBUG
        qDebug() << "appSharedSecret = " << sharedSecret;
    #endif
    _appSharedSecret = sharedSecret;
}

QString QDropbox::sharedSecret() const
{
    return _appSharedSecret;
}

void QDropbox::setToken(QString t)
{
    #ifdef QTDROPBOX_DEBUG
        qDebug() << "oauthToken = " << t;
    #endif
    oauthToken = t;
}

QString QDropbox::token() const
{
    return oauthToken;
}

void QDropbox::setTokenSecret(QString s)
{
    #ifdef QTDROPBOX_DEBUG
        qDebug() << "oauthTokenSecret = " << oauthTokenSecret;
    #endif
    oauthTokenSecret = s;
}

QString QDropbox::tokenSecret() const
{
    return oauthTokenSecret;
}

QString QDropbox::appKey() const
{
    return _appKey;
}

QString QDropbox::appSharedSecret() const
{
    return _appSharedSecret;
}

QString QDropbox::apiVersion() const
{
    return _version;
}

int QDropbox::requestToken(bool blocking)
{
	clearError();
    QString signature_method_string = signatureMethodString();

    timestamp = QDateTime::currentMSecsSinceEpoch()/1000;
    nonce = generateNonce(128);

    QUrl url;
    url.setUrl(apiurl.toString());
    url.setPath(QString("/%1/oauth/request_token").arg(_version.left(1)));

    QUrlQuery query;
    query.addQueryItem("oauth_consumer_key", _appKey);
    query.addQueryItem("oauth_nonce", nonce);
    query.addQueryItem("oauth_signature_method", signature_method_string);
    query.addQueryItem("oauth_timestamp", QString::number(timestamp));
    query.addQueryItem("oauth_version", _version);

    QString signature = oAuthSign(url);
    query.addQueryItem("oauth_signature", QUrl::toPercentEncoding(signature));

    url.setQuery(query);
    #ifdef QTDROPBOX_DEBUG
        qDebug() << "request token url: " << url.toString() << endl << "sig: " << signature;
        qDebug() << "sending request " << url.toString() << " to " << apiurl.toString();
    #endif

    int reqnr = sendRequest(url, "GET");
    if(blocking)
    {
        requestMap[reqnr].type = QDROPBOX_REQ_RQBTOKN;
        startEventLoop();
    }
    else
        requestMap[reqnr].type = QDROPBOX_REQ_RQTOKEN;

    return reqnr;
}

bool QDropbox::requestTokenAndWait()
{
    requestToken(true);
    return (error() == NoError);
}

int QDropbox::authorize(QString email, QString pwd)
{
    QUrl dropbox_authorize;
    dropbox_authorize.setPath(QString("/%1/oauth/authorize")
                              .arg(_version.left(1)));
    #ifdef QTDROPBOX_DEBUG
        qDebug() << "oauthToken = " << oauthToken << endl;
    #endif

    QUrlQuery query;
    query.addQueryItem("oauth_token", oauthToken);
    dropbox_authorize.setQuery(query);

    int reqnr = sendRequest(dropbox_authorize, "GET", 0, "www.dropbox.com");
    requestMap[reqnr].type = QDROPBOX_REQ_AULOGIN;
    mail     = email;
    password = pwd;
    return reqnr;
}

QUrl QDropbox::authorizeLink()
{
    QUrl link;
    link.setScheme("https");
    link.setHost("www.dropbox.com");
    link.setPath(QString("/%1/oauth/authorize")
                 .arg(_version.left(1)));

    QUrlQuery query;
    query.addQueryItem("oauth_token", oauthToken);
    link.setQuery(query);
    return link;
}

int QDropbox::requestAccessToken(bool blocking)
{
	clearError();

    QUrl url;
    url.setUrl(apiurl.toString());

    QUrlQuery query;
    query.addQueryItem("oauth_consumer_key",_appKey);
    query.addQueryItem("oauth_nonce", nonce);
    query.addQueryItem("oauth_signature_method", signatureMethodString());
    query.addQueryItem("oauth_timestamp", QString::number(timestamp));
    query.addQueryItem("oauth_token", oauthToken);
    query.addQueryItem("oauth_version", _version);

    url.setPath(QString("/%1/oauth/access_token").
                arg(_version.left(1)));

    #ifdef QTDROPBOX_DEBUG
        qDebug() << "requestToken = " << query.queryItemValue("oauth_token");
    #endif

    QString signature = oAuthSign(url);
    query.addQueryItem("oauth_signature", QUrl::toPercentEncoding(signature));

    url.setQuery(query);

    QString dataString = url.toString(QUrl::RemoveScheme|QUrl::RemoveAuthority|
                                      QUrl::RemovePath).mid(1);
    #ifdef QTDROPBOX_DEBUG
        qDebug() << "dataString = " << dataString << endl;
    #endif

    QByteArray postData;
    postData.append(dataString.toUtf8());

    QUrl xQuery(url.toString(QUrl::RemoveQuery));
    int reqnr = sendRequest(xQuery, "POST", postData);

    if(blocking)
    {
        requestMap[reqnr].type = QDROPBOX_REQ_BACCTOK;
        startEventLoop();
    }
    else
        requestMap[reqnr].type = QDROPBOX_REQ_ACCTOKN;

    return reqnr;
}

bool QDropbox::requestAccessTokenAndWait()
{
    requestAccessToken(true);

    #ifdef QTDROPBOX_DEBUG
        qDebug() << "requestTokenAndWait() finished: error = " << error() << endl;
    #endif

    return (error() == NoError);
}

void QDropbox::requestAccountInfo(bool blocking)
{
    clearError();

    timestamp = QDateTime::currentMSecsSinceEpoch()/1000;

    QUrl url (apiurl);
    //url.setUrl(apiurl.toString());

    QUrlQuery urlQuery;
    urlQuery.addQueryItem("oauth_consumer_key",_appKey);
    urlQuery.addQueryItem("oauth_nonce", nonce);
    urlQuery.addQueryItem("oauth_signature_method", signatureMethodString());
    urlQuery.addQueryItem("oauth_timestamp", QString::number(timestamp));
    urlQuery.addQueryItem("oauth_token", oauthToken);
    urlQuery.addQueryItem("oauth_version", _version);

    QString signature = oAuthSign(url);
    urlQuery.addQueryItem("oauth_signature", QUrl::toPercentEncoding(signature));

    url.setPath(QString("/%1/account/info").arg(_version.left(1)));
    url.setQuery(urlQuery);

    int reqnr = sendRequest(url);
    if(blocking)
    {
        requestMap[reqnr].type = QDROPBOX_REQ_BACCINF;
        startEventLoop();
    }
    else
        requestMap[reqnr].type = QDROPBOX_REQ_ACCINFO;
    return;
}

QDropboxAccount QDropbox::requestAccountInfoAndWait()
{
    requestAccountInfo(true);
    QDropboxAccount a(_tempJson.strContent(), this);
    _account = a;
    return _account;
}

void QDropbox::parseBlockingAccountInfo(QString response)
{
    clearError();
    parseAccountInfo(response);
    stopEventLoop();
}

void QDropbox::requestMetadata(QString file, bool blocking)
{
    clearError();

    timestamp = QDateTime::currentMSecsSinceEpoch()/1000;

    QUrl url;
    url.setUrl(apiurl.toString());

    QUrlQuery urlQuery;
    urlQuery.addQueryItem("oauth_consumer_key",_appKey);
    urlQuery.addQueryItem("oauth_nonce", nonce);
    urlQuery.addQueryItem("oauth_signature_method", signatureMethodString());
    urlQuery.addQueryItem("oauth_timestamp", QString::number(timestamp));
    urlQuery.addQueryItem("oauth_token", oauthToken);
    urlQuery.addQueryItem("oauth_version", _version);

    QString signature = oAuthSign(url);
    urlQuery.addQueryItem("oauth_signature", QUrl::toPercentEncoding(signature));

    url.setQuery(urlQuery);
    url.setPath(QString("/%1/metadata/%2").arg(_version.left(1), file));

    int reqnr = sendRequest(url);
    if(blocking)
    {
        requestMap[reqnr].type = QDROPBOX_REQ_BMETADA;
        startEventLoop();
    }
    else
        requestMap[reqnr].type = QDROPBOX_REQ_METADAT;
}

QDropboxFileInfo QDropbox::requestMetadataAndWait(QString file)
{
    requestMetadata(file, true);
    QDropboxFileInfo file_info(_tempJson.strContent(), this);

    #ifdef QTDROPBOX_DEBUG
        qDebug() << "QDropbox::requestMetadataAndWait. File name: " << file;
        qDebug() << "Size: " << file_info.size();
        qDebug() << "Icon name: " << file_info.icon() << "Path: " << file_info.path();
    #endif

    return file_info;
}

void QDropbox::requestSharedLink(QString file, bool blocking)
{
	clearError();

    QUrl url;
    url.setUrl(apiurl.toString());

    QUrlQuery urlQuery;
    urlQuery.addQueryItem("oauth_consumer_key",_appKey);
    urlQuery.addQueryItem("oauth_nonce", nonce);
    urlQuery.addQueryItem("oauth_signature_method", signatureMethodString());
    urlQuery.addQueryItem("oauth_timestamp", QString::number(timestamp));
    urlQuery.addQueryItem("oauth_token", oauthToken);
    urlQuery.addQueryItem("oauth_version", _version);

    QString signature = oAuthSign(url);
    urlQuery.addQueryItem("oauth_signature", QUrl::toPercentEncoding(signature));

    url.setPath(QString("/%1/shares/%2").arg(_version.left(1), file));
    url.setQuery(urlQuery);

    int reqnr = sendRequest(url);
    if(blocking)
    {
        requestMap[reqnr].type = QDROPBOX_REQ_BSHRDLN;
        startEventLoop();
    }
    else
        requestMap[reqnr].type = QDROPBOX_REQ_SHRDLNK;

    return;
}

QUrl QDropbox::requestSharedLinkAndWait(QString file)
{
    requestSharedLink(file,true);
    QDropboxJson json(_tempJson.strContent());
    QString urlString = json.getString("url");
    return QUrl(urlString);
}

void QDropbox::requestDelta(QString cursor, QString path_prefix, bool blocking)
{
    clearError();

    timestamp = QDateTime::currentMSecsSinceEpoch()/1000;

    QUrl url;
    url.setUrl(apiurl.toString());

    QUrlQuery urlQuery;
    urlQuery.addQueryItem("oauth_consumer_key", _appKey);
    urlQuery.addQueryItem("oauth_nonce", nonce);
    urlQuery.addQueryItem("oauth_signature_method", signatureMethodString());
    urlQuery.addQueryItem("oauth_timestamp", QString::number(timestamp));
    urlQuery.addQueryItem("oauth_token", oauthToken);
    urlQuery.addQueryItem("oauth_version", _version);

    if(cursor.length() > 0)
    {
        urlQuery.addQueryItem("cursor", cursor);
    }
    if(path_prefix.length() > 0)
    {
        urlQuery.addQueryItem("path_prefix", path_prefix);
    }

    QString signature = oAuthSign(url);
    urlQuery.addQueryItem("oauth_signature", QUrl::toPercentEncoding(signature));

    url.setQuery(urlQuery);
    url.setPath(QString("/%1/delta").arg(_version.left(1)));
    /* The path is the part of the URL that comes after the authority but before the query string. IE: ftp://ftp.example.com{/pub/something/}  (dir in brackets is the path)*/

    QString dataString = url.toString(QUrl::RemoveScheme|QUrl::RemoveAuthority|
                                      QUrl::RemovePath).mid(1);

    //"oauth_consumer_key=hj0l48dp8hsgoyf&oauth_nonce&oauth_signature_method=PLAINTEXT&oauth_timestamp=1468355099&oauth_token=15kpr51pllz4rzzc&oauth_version=1.0&oauth_signature=v9tyo7cplqvqoox%26jt5icz1q68uylw5"
    QByteArray postData;
    postData.append(dataString.toUtf8());

    #ifdef QTDROPBOX_DEBUG
        qDebug() << "postData: " << postData;
    #endif

    QUrl xQuery(url.toString(QUrl::RemoveQuery)); //https://api.dropbox.com/1/delta
    int reqnr = sendRequest(xQuery, "POST", postData);

    #ifdef QTDROPBOX_DEBUG
        qDebug() << "xQuery: " << xQuery.toString();
    #endif

    if(blocking)
    {
        requestMap[reqnr].type = QDROPBOX_REQ_BDELTA;
        startEventLoop();
    }
    else
        requestMap[reqnr].type = QDROPBOX_REQ_DELTA;
}

QDropboxDeltaResponse QDropbox::requestDeltaAndWait(QString cursor, QString path_prefix)
{
    requestDelta(cursor, path_prefix, true);
    QDropboxDeltaResponse delta_response(_tempJson.strContent());

    return delta_response;
}

void QDropbox::startEventLoop()
{
    /* TO DO: Try changing evLoop to smart pointer. */
    if(_evLoop == NULL)
        _evLoop = new QEventLoop(this);
    _evLoop->exec();
}

void QDropbox::stopEventLoop()
{
    if(_evLoop == NULL)
        return;

    _evLoop->exit();
}

void QDropbox::responseBlockedTokenRequest(QString response)
{
    clearError();
    responseTokenRequest(response);
    stopEventLoop();
}

void QDropbox::responseBlockingAccessToken(QString response)
{
    clearError();
    responseAccessToken(response);
    stopEventLoop();
}

void QDropbox::parseBlockingMetadata(QString response)
{
    clearError();
    parseMetadata(response);
    stopEventLoop();
}

void QDropbox::parseBlockingDelta(QString response)
{
    clearError();
    parseDelta(response);
    stopEventLoop();
}

void QDropbox::parseBlockingSharedLink(QString response)
{
    clearError();
    parseSharedLink(response);
    stopEventLoop();
}

// check if the event loop has to be stopped after a blocking request was sent
void QDropbox::checkReleaseEventLoop(int reqnr)
{
    switch(requestMap[reqnr].type)
    {
    /* TO DO: Are all of those cases needed? I think we could just have here case QDROPBOX_REQ_BREVISI and default case. */
    case QDROPBOX_REQ_RQBTOKN:
    case QDROPBOX_REQ_BACCTOK:
    case QDROPBOX_REQ_BACCINF:
    case QDROPBOX_REQ_BMETADA:
	case QDROPBOX_REQ_BREVISI:
        stopEventLoop(); // release local event loop
        break;
    default:
        break;
    }
}

void QDropbox::requestRevisions(QString file, int max, bool blocking)
{
	clearError();

	QUrl url;
    url.setUrl(apiurl.toString());

    QUrlQuery urlQuery;
    urlQuery.addQueryItem("oauth_consumer_key",_appKey);
    urlQuery.addQueryItem("oauth_nonce", nonce);
    urlQuery.addQueryItem("oauth_signature_method", signatureMethodString());
    urlQuery.addQueryItem("oauth_timestamp", QString::number(timestamp));
    urlQuery.addQueryItem("oauth_token", oauthToken);
    urlQuery.addQueryItem("oauth_version", _version);
    urlQuery.addQueryItem("rev_limit", QString::number(max));

    QString signature = oAuthSign(url);
    urlQuery.addQueryItem("oauth_signature", QUrl::toPercentEncoding(signature));

    url.setPath(QString("/%1/revisions/%2").arg(_version.left(1), file));
    url.setQuery(urlQuery);

    int reqnr = sendRequest(url);
    if(blocking)
    {
        requestMap[reqnr].type = QDROPBOX_REQ_BREVISI;
        startEventLoop();
    }
    else
        requestMap[reqnr].type = QDROPBOX_REQ_REVISIO;
}

QList<QDropboxFileInfo> QDropbox::requestRevisionsAndWait(QString file, int max)
{
	clearError();
	requestRevisions(file, max, true);
	QList<QDropboxFileInfo> revisionList;

	if(errorState != QDropbox::NoError || !_tempJson.isValid())
		return revisionList;

	QStringList responseList = _tempJson.getArray();
    for(int i = 0; i < responseList.size(); ++i)
	{
		QString revData = responseList.at(i);
		QDropboxFileInfo revision(revData);
		revisionList.append(revision);
	}

	return revisionList;
}

void QDropbox::parseRevisions(QString response)
{
    QDropboxJson json;
    _tempJson.parseString(response);
    if(!_tempJson.isValid())
    {
        errorState = QDropbox::APIError;
        errorText  = "Dropbox API did not send correct answer for file/directory metadata.";

        emit errorOccured(errorState);
        stopEventLoop();
        return;
    }

    emit revisionsReceived(response);
}

void QDropbox::parseBlockingRevisions(QString response)
{
	clearError();
	parseRevisions(response);
	stopEventLoop();
}

void QDropbox::clearError()
{
    errorState  = QDropbox::NoError;
    errorText  = "";
}

qdropbox_request QDropbox::requestInfo(int rqnr)
{
    if (requestMap.contains(rqnr) == false)
        return { QDROPBOX_REQ_INVALID, "", "", 0 }; // invalid request. If class doesn't have a constructor, we can use aggregate initialization.

	return requestMap[rqnr];
}

void QDropbox::removeRequestFromMap(int rqnr)
{
    if (saveFinishedRequests() == false)
		requestMap.remove(rqnr);
}

void QDropbox::setSaveFinishedRequests(bool save)
{
	_saveFinishedRequests = save;
}

bool QDropbox::saveFinishedRequests() const
{
	return _saveFinishedRequests;
}


void QDropbox::Download_File()
{
    clearError();

    QUrl url;
    url.setUrl(apiurl.toString());

}
