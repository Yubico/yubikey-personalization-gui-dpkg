/*
 * Copyright (c) 2011 Yubico AB
 * See the file COPYING for licence statement.
 *
 */

#ifndef OATHPAGE_H
#define OATHPAGE_H

#include <QStackedWidget>
#include "yubikeyconfig.h"
#include "common.h"

namespace Ui {
    class OathPage;
}

class OathPage : public QStackedWidget {
    Q_OBJECT

public:
    explicit OathPage(QWidget *parent = 0);
    ~OathPage();

private:
    Ui::OathPage *ui;

    enum Page {
        Page_Base,
        Page_Quick,
        Page_Advanced
    };
    int m_currentPage;

    int m_customerPrefix;
    unsigned char m_pubId[OATH_HOTP_PUBLIC_ID_SIZE];
    int m_pubIdFormat;

    enum State {
        State_Initial,
        State_Programming,
        State_Programming_Multiple,
        State_Programming_Multiple_Auto
    };
    State m_state;
    YubiKeyConfig *m_ykConfig;
    unsigned int m_keysProgrammedCtr;
    bool m_ready;
    bool m_serialNumberSupported;

public slots:
    void loadSettings();

private slots:
    void connectPages();
    void connectHelpButtons();
    void setCurrentPage(int pageIndex);
    void helpBtn_pressed(int helpIndex);
    void keyFound(bool found, bool* featuresMatrix);

    void updatePrefix();
    void fixBCD(unsigned char *bp, int bcnt);
    void clearState();

    // Quick Page
    void resetQuickPage();
    void on_quickResetBtn_clicked();

    void on_quickHideParams_clicked(bool checked);
    void on_quickPubIdCheck_stateChanged(int state);
    void resetQuickPrefix();
    void updateQuickMUI();
    void on_quickMUITxt_editingFinished();
    void on_quickMUIGenerateBtn_clicked();
    void on_quickSecretKeyTxt_editingFinished();

    bool validateQuickSettings();
    void writeQuickConfig();
    void quickConfigWritten(bool written, const QString &msg);

    // Advanced Page
    void resetAdvPage();
    void freezeAdvPage(bool freeze);

    void on_advProgramMulKeysBox_clicked(bool checked);
    void on_advConfigParamsCombo_currentIndexChanged(int index);

    void on_advConfigProtectionCombo_currentIndexChanged(int index);
    void on_advCurrentAccessCodeTxt_editingFinished();
    void on_advNewAccessCodeTxt_editingFinished();

    void on_advPubIdFormatCombo_currentIndexChanged(int index);
    void on_advPubIdCheck_stateChanged(int state);
    void updateAdvOMP(int index);
    void on_advOMPTxt_editingFinished();
    void updateAdvTT(int index);
    void on_advTTTxt_editingFinished();
    void updateAdvMUI(int index);
    void on_advMUITxt_editingFinished();
    void on_advMUIGenerateBtn_clicked();

    void on_advMovingFactorSeedCombo_currentIndexChanged(int index);
    void on_advMovingFactorSeedTxt_editingFinished();

    void on_advSecretKeyTxt_editingFinished();
    void on_advSecretKeyGenerateBtn_clicked();

    void on_advWriteConfigBtn_clicked();
    void on_advStopBtn_clicked();

    void changeAdvConfigParams();
    void stopAdvConfigWritting();
    bool validateAdvSettings();
    void writeAdvConfig();
    void advConfigWritten(bool written, const QString &msg);

    void advUpdateResults(bool written, const QString &msg);

signals:
    void showStatusMessage(const QString &text, int status = 0);
};

#endif // OATHPAGE_H