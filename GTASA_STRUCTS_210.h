#ifndef _GTASA_STRUCTS_210_H
#define _GTASA_STRUCTS_210_H

#include <stdint.h>

class CVector
{
public:
    float x, y, z;
    CVector() : x(0.0f), y(0.0f), z(0.0f) {}
    CVector(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
    
    float Magnitude() const { return 0.0f; }
};

class CVector2D
{
public:
    float x, y;
};

class CRGBA
{
public:
    uint8_t r, g, b, a;
};

class CMatrix
{
public:
    CVector pos;
    CVector GetRight() { return CVector(1,0,0); }
    CVector GetForward() { return CVector(0,1,0); }
    CVector GetUp() { return CVector(0,0,1); }
};

class CEntity
{
public:
    void*    vtable;
    void*    m_rwObject;
    uint32_t m_nType      : 3;
    uint32_t m_nStatus     : 5;
    uint8_t  _pad0[0x18];
    int16_t  m_nModelIndex;
    int16_t  _pad1;
    CMatrix* m_matrix;

    CVector GetPosition() { return m_matrix ? m_matrix->pos : CVector(); }
};

class CColModel
{
public:
    struct {
        CVector m_vecMin;
        CVector m_vecMax;
    } m_boxBound;
};

class CColPoint
{
public:
    CVector m_vecPoint;
};

class CPed : public CEntity
{
public:
    bool IsPlayer() { return false; }
};

class CPlayerPed : public CPed
{
public:
    class CVehicle* m_pVehicle;
};

class CVehicle : public CEntity
{
public:
    CPed* m_pDriver;
    uint8_t m_nCreateBy;
    uint8_t m_nVehicleSubType;
    float m_fMovingSpeed;
    CVector m_vecMoveSpeed;
    float m_fSteerAngle;
    int m_nCurrentGear;
    float m_fGasPedal;
    
    struct {
        uint8_t bSirenOrAlarm : 1;
        uint8_t bParking : 1;
        uint8_t bIsBig : 1;
        uint8_t bIsBus : 1;
        uint8_t bIsLawEnforcer : 1;
        uint8_t bIsAmbulanceOnDuty : 1;
        uint8_t bIsFireTruckOnDuty : 1;
    } vehicleFlags;

    struct {
        uint8_t CruiseSpeed;
        uint8_t DrivingMode;
        int NewLane;
        int OldLane;
        bool bAlwaysInSlowLane;
        CEntity* pTargetEntity;
    } m_AutoPilot;

    void* m_pHandling;

    int GetNumContactWheels() { return 4; }
    void Teleport(CVector pos) {}
};

class CAutomobile : public CVehicle {};
class CBike : public CVehicle {};
class CHeli : public CVehicle {};
class CPlane : public CVehicle {};
class CQuad : public CVehicle {};

class CObject : public CEntity {};
class CCutsceneObject : public CObject {};

template <typename T, typename T2 = void>
class CPool
{
public:
    char* _pad0;
    T* m_pObjects;
    int m_nSize;
    int m_nFirstFree;
    struct {
        uint8_t bEmpty : 1;
    } *m_byteMap;

    T* GetAt(int index) { return &m_pObjects[index]; }
    int GetIndex(T* obj) { return 0; }
};

struct RwCamera
{
    uint8_t  _pad0[0xa8];
    float    nearClip;
    float    farClip;
    uint8_t  _pad1[0x50];
};

class CCamera
{
public:
    uint8_t   _pad0[0x930];
    RwCamera* m_pRwCamera;
    uint8_t   _pad1[0x100];
    
    CVector GetPosition() { return CVector(); }
};

class ES2Shader
{
public:
    int      nShaderId;
    uint32_t flags;
    uint8_t  _pad0[0x48];
};

float DistanceBetweenPoints(const CVector& v1, const CVector& v2);

#endif // _GTASA_STRUCTS_210_H
