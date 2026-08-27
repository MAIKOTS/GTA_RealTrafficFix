// ============================================================================
// GTASA_STRUCTS.h (Ajustado)
// ============================================================================

#ifndef _GTASA_STRUCTS_H
#define _GTASA_STRUCTS_H

#include <stdint.h>

enum eVehicleCreatedBy
{
    RANDOM_VEHICLE = 1,
    MISSION_VEHICLE = 2,
    PARKED_VEHICLE = 3
};

enum eVehicleType
{
    VEHICLE_TYPE_AUTOMOBILE = 0,
    VEHICLE_TYPE_MTRUCK = 1,
    VEHICLE_TYPE_QUAD = 2,
    VEHICLE_TYPE_HELI = 3,
    VEHICLE_TYPE_BOAT = 4,
    VEHICLE_TYPE_PLANE = 5,
    VEHICLE_TYPE_BIKE = 6,
    VEHICLE_TYPE_BMX = 7,
    VEHICLE_TYPE_TRAILER = 8
};

enum eDrivingStyle
{
    DRIVING_STYLE_STOP_FOR_CARS = 0,
    DRIVING_STYLE_AVOID_CARS = 1,
    DRIVING_STYLE_PLOUGH_THROUGH = 2,
    DRIVING_STYLE_STOP_FOR_CARS_IGNORE_LIGHTS = 3,
    DRIVING_STYLE_AVOID_CARS_STOP_FOR_PEDS_OBEY_LIGHTS = 4
};

struct tTransmissionData {
    float m_fMaxVelocity;
};

struct tHandlingData {
    tTransmissionData Transmission;
};

class CVector
{
public:
    float x, y, z;
    CVector() : x(0.0f), y(0.0f), z(0.0f) {}
    CVector(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
    
    CVector operator+(const CVector& v) const {
        return CVector(x + v.x, y + v.y, z + v.z);
    }

    CVector operator-(const CVector& v) const {
        return CVector(x - v.x, y - v.y, z - v.z);
    }
    
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
    uint8_t  _pad0[0x14];
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

    tHandlingData* m_pHandling;

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

    CPool(int size, const char* name) {
        m_nSize = size;
        m_pObjects = new T[size];
    }

    T* GetAt(int index) { return &m_pObjects[index]; }
    
    int GetIndex(T* obj) {
        return (int)(obj - m_pObjects);
    }
};

struct RwCamera
{
    uint8_t  _pad0[0x9C];
    float    nearClip;
    float    farClip;
};

class CCamera
{
public:
    uint8_t   _pad0[0x18];
    RwCamera* m_pRwCamera;
    
    CVector GetPosition() { return CVector(); }
};

class ES2Shader
{
public:
    int      nShaderId;
    uint32_t flags;
    uint8_t  _pad0[0x40];
};

float DistanceBetweenPoints(const CVector& v1, const CVector& v2);

#endif // _GTASA_STRUCTS_H
