#ifndef CARLAPATCHBACKEND_H
#define CARLAPATCHBACKEND_H

#include "abstractpatchbackend.hpp"

class CarlaPatchBackend : public AbstractPatchBackend
{
        Q_OBJECT
    
public:
    CarlaPatchBackend(const QString& patchfile);
    
    
public slots:
    
    void kill() override;
    void preload() override;
    void activate() override;
    void deactivate() override;
    
    
private:
    ~CarlaPatchBackend(){}
};

#endif
