/*
 * Copyright (c) 2011 Yubico AB
 * See the file COPYING for licence statement.
 *
 */

#include "yubikeyfinder.h"

#define YK_VERSION(MAJOR, MINOR, BUILD) (MAJOR * 100 + MINOR * 10 + BUILD)

YubiKeyFinder* YubiKeyFinder::_instance = 0;

const unsigned int YubiKeyFinder::FEATURE_MATRIX[][2] = {
    { YK_VERSION(2,0,0), 0 },   //Feature_MultipleConfigurations
    { YK_VERSION(2,0,0), 0 },   //Feature_ProtectConfiguration2
    { YK_VERSION(1,3,0), 0 },   //Feature_StaticPassword
    { YK_VERSION(2,0,0), 0 },   //Feature_ScanCodeMode
    { YK_VERSION(2,0,0), 0 },   //Feature_ShortTicket
    { YK_VERSION(2,0,0), 0 },   //Feature_StrongPwd
    { YK_VERSION(2,1,0), 0 },   //Feature_OathHotp
    { YK_VERSION(2,2,0), 0 },   //Feature_ChallengeResponse
    { YK_VERSION(2,2,0), 0 },   //Feature_SerialNumber
    { YK_VERSION(2,2,0), 0 }    //Feature_MovingFactor
};

YubiKeyFinder::YubiKeyFinder() {
    //Initialize fields
    init();

    //Create timer
    m_timer = new QTimer( this );
    connect(m_timer, SIGNAL(timeout()),
            this, SLOT(findKey()));
}

YubiKeyFinder::~YubiKeyFinder() {
    closeKey();

    if(m_timer != 0) {
        delete m_timer;
        m_timer = 0;
    }

    if(_instance) {
        delete _instance;
    }
}

YubiKeyFinder* YubiKeyFinder::getInstance() {
    if(_instance == NULL) {
        _instance = new YubiKeyFinder();
    }
    return _instance;
}

QString YubiKeyFinder::versionStr() {
    if(m_version > 0) {
        return tr("%1.%2.%3").
                arg(m_versionMajor).
                arg(m_versionMinor).
                arg(m_versionBuild);
    }
    return "";
}

void YubiKeyFinder::reportError() {
    if (ykp_errno) {
        //qDebug("Yubikey personalization error: %s\n", ykp_strerror(ykp_errno));
        ykp_errno = 0;
    } else if (yk_errno) {
        if (yk_errno == YK_EUSBERR) {
            //qDebug("USB error: %s\n", yk_usb_strerror());
        } else {
            //qDebug("Yubikey core error: %s\n", yk_strerror(yk_errno));
        }

        yk_errno = 0;
    }

    emit errorOccurred(ERR_KEY_NOT_FOUND);
}

bool YubiKeyFinder::checkFeatureSupport(Feature feature) {
    if(m_version > 0 &&
       (unsigned int) feature < sizeof(FEATURE_MATRIX)/sizeof(FEATURE_MATRIX[0])) {
        return (
                m_version >= FEATURE_MATRIX[feature][0] &&
                (FEATURE_MATRIX[feature][1] == 0 || m_version <= FEATURE_MATRIX[feature][1])
                );
    }

    return false;
}

void YubiKeyFinder::init() {
    m_state = State_Absent;
    m_yk = 0;
    m_version = 0;
    m_versionMajor = 0;
    m_versionMinor = 0;
    m_versionBuild = 0;
    m_serial = 0;
}

void YubiKeyFinder::start() {
    //Start timer
    if(m_timer && !m_timer->isActive()) {
        m_timer->start(TIMEOUT_FINDER);
    }
}

void YubiKeyFinder::stop() {
    //Stop timer
    if(m_timer && m_timer->isActive()) {
        m_timer->stop();
    }
    //closeKey();
}

bool YubiKeyFinder::openKey() {
    bool flag = true;
    if(m_yk != 0) {
        closeKey();
    }

    if (!yk_init()) {
        flag = false;
    } else if (!(m_yk = yk_open_first_key())) {
        flag = false;
    }

    return flag;
}

bool YubiKeyFinder::closeKey() {
    bool flag = true;
    if(m_yk != 0) {
        if (!yk_close_key(m_yk)) {
            flag = false;
        }
        if (!yk_release()) {
            flag = false;
        }
    }

    init();
    return flag;
}

void YubiKeyFinder::findKey() {
    YK_STATUS *ykst = ykds_alloc();
    bool error = false;

    //qDebug() << "-------------------------";
    //qDebug() << "Starting key search";
    //qDebug() << "-------------------------";

    try {
        if(m_yk == 0 && !openKey()) {
            throw 0;
        }

        if (!yk_get_status(m_yk, ykst)) {
            throw 0;
        }

        //qDebug() << "Key found";

        //Check pervious state
        if(m_state == State_Absent) {

            m_state = State_Preset;

            //Get version
            m_versionMajor = ykds_version_major(ykst);
            m_versionMinor = ykds_version_minor(ykst);
            m_versionBuild = ykds_version_build(ykst);
            m_version = YK_VERSION(m_versionMajor,
                                   m_versionMinor,
                                   m_versionBuild);

            //Get serial number
            if(checkFeatureSupport(Feature_SerialNumber)) {
                if (!yk_get_serial(m_yk, 0, 0, &m_serial)) {
                    qDebug() << "Failed to read serial number (serial-api-visible disabled?).";
                } else {
                    qDebug() << "Serial number: " << m_serial;
                }
            }

            //Get supported features
            size_t featuresCount = sizeof(FEATURE_MATRIX)/sizeof(FEATURE_MATRIX[0]);
            bool featuresMatrix[featuresCount];
            for(size_t i = 0; i < featuresCount; i++) {
                featuresMatrix[i] = checkFeatureSupport((Feature)i);
            }

            emit keyFound(true, featuresMatrix);
        }
    }
    catch(...) {
        error = true;
    }

    if(ykst) {
        ykds_free(ykst);
    }

    if(error) {
        m_state = State_Absent;
        closeKey();
        reportError();
        emit keyFound(false, NULL);
    }

    //qDebug() << "-------------------------";
    //qDebug() << "Stopping key search";
    //qDebug() << "-------------------------";
}