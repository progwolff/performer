#ifndef ANDROIDFILEDIALOG_H
#define ANDROIDFILEDIALOG_H

#include <QObject>
#include <QAndroidJniObject>
#include <QtAndroid>
#include <QAndroidActivityResultReceiver>

class AndroidFileDialog : public QObject
{
    Q_OBJECT

public:
    explicit AndroidFileDialog(QObject *parent = 0);
    virtual ~AndroidFileDialog();
    bool provideExistingFileName();

private:
    class ResultReceiver : public QAndroidActivityResultReceiver {
        AndroidFileDialog *_dialog;
    public:
        ResultReceiver(AndroidFileDialog *dialog);
        virtual ~ResultReceiver();
        void handleActivityResult(int receiverRequestCode, int resultCode, const QAndroidJniObject &data);
        QString uriToPath(QAndroidJniObject uri);
    };

    static const int EXISTING_FILE_NAME_REQUEST = 1;
    ResultReceiver *receiver;
    void emitExistingFileNameReady(QString result);

signals:
    void existingFileNameReady(QString result);
};

#endif // ANDROIDFILEDIALOG_H
