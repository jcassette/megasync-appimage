#ifndef MACXLOCALSOCKET_H
#define MACXLOCALSOCKET_H

#include <QObject>
#include <objc/runtime.h>

class MacXLocalSocketPrivate;
class MacXLocalSocket : public QObject
{
    Q_OBJECT

public:
    MacXLocalSocket(MacXLocalSocketPrivate *clientSocketPrivate);
    ~MacXLocalSocket();

    qint64 readCommand(QByteArray *data);

    //This method is called from two different threads, but it is thread-safe
    bool writeData(const char * data, int len);
    void appendDataToBuffer(QByteArray data);

signals:
    void dataReady(QByteArray data);

private:
    MacXLocalSocketPrivate* socketPrivate;
};

#endif // MACXLOCALSOCKET_H
