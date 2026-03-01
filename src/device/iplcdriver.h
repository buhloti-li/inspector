#ifndef IPLCDRIVER_H
#define IPLCDRIVER_H

#include "idevicedriver.h"
#include <functional>

class IPLCDriver : public IDeviceDriver {
public:
    QString deviceType() const override { return QStringLiteral("PLC"); }

    virtual QVariant readTag(const QString& tagName) = 0;
    virtual bool writeTag(const QString& tagName, const QVariant& value) = 0;
    virtual bool subscribeTag(const QString& tagName,
                              std::function<void(const QVariant&)> callback) = 0;
    virtual void unsubscribeTag(const QString& tagName) = 0;
};

#endif // IPLCDRIVER_H
