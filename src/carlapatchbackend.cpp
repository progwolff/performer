#include "carlapatchbackend.h"


CarlaPatchBackend::CarlaPatchBackend(const QString& patchfile)
    : AbstractPatchBackend(patchfile)
{
    emit progress(0);
}

void CarlaPatchBackend::kill()
{
    deleteLater();
}

void CarlaPatchBackend::preload()
{
}

void CarlaPatchBackend::activate()
{
}

void CarlaPatchBackend::deactivate()
{
}
