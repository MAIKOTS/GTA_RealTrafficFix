// GetExtData (mais defensiva)
inline ExtendedVehicleData* GetExtData(CVehicle* veh)
{
    // Se ainda não inicializamos o pool extendido, tente criar apenas se o pool do jogo existir
    if(!ms_pVehicleExtendedPool)
    {
        if(!ms_pVehiclePool || !*ms_pVehiclePool) return nullptr;
        auto size = (*ms_pVehiclePool)->m_nSize;
        ms_pVehicleExtendedPool = new CPool<ExtendedVehicleData>(size, "ExtendedVehicleData");
        ms_pVehicleExtendedPool->m_nFirstFree = size;
        for(int i = 0; i < size; ++i) ms_pVehicleExtendedPool->m_byteMap[i].bEmpty = false;
    }

    // Validações adicionais: pool do jogo e índice válidos
    if(!ms_pVehiclePool || !*ms_pVehiclePool) return nullptr;
    int idx = (*ms_pVehiclePool)->GetIndex(veh);
    if(idx < 0 || idx >= (int)(*ms_pVehiclePool)->m_nSize) return nullptr;

    return ms_pVehicleExtendedPool->GetAt(idx);
}

// ProcessRTFVehicle (usa ponteiro e valida antes de acessar)
inline void ProcessRTFVehicle(CVehicle* vehicle)
{
    if (!vehicle->m_pDriver || vehicle->m_nCreateBy == eVehicleCreatedBy::MISSION_VEHICLE || vehicle->m_pDriver->IsPlayer()) return;

    auto subClass = vehicle->m_nVehicleSubType;
    bool isBike = (subClass == VEHICLE_TYPE_BIKE || subClass == VEHICLE_TYPE_BMX);
    if (subClass != VEHICLE_TYPE_AUTOMOBILE &&
        subClass != VEHICLE_TYPE_MTRUCK &&
        subClass != VEHICLE_TYPE_QUAD &&
        !isBike) return;

    ExtendedVehicleData* xdata = GetExtData(vehicle);
    if(!xdata) return; // Se não for possível obter dados estendidos, aborta

    float curZOffset = 0.0f;
    float fNewCruiseSpeed = CruiseSpeed;

    // Cruise default speed
    vehicle->m_AutoPilot.CruiseSpeed = (uint8_t)fNewCruiseSpeed;

    // Use real physics
    if (xdata->flags.bPhysics == false)
    {
        if(subClass == VEHICLE_TYPE_AUTOMOBILE ||
           subClass == VEHICLE_TYPE_BMX ||
           subClass == VEHICLE_TYPE_MTRUCK)
        {
            SwitchVehicleToRealPhysics(vehicle);
            xdata->flags.bPhysics = true;
        }
    }

    CVector modelMin;
    CVector modelMax;
    GetEntityDimensions(vehicle, &modelMin, &modelMax);

    // Ground stuck
    if (xdata->flags.bRunGroundStuckFix == true && DoFixGroundStuck)
    {
        if (((vehicle->m_fMovingSpeed * (*ms_fTimeStep * fMagic)) < 0.5f) && FixGroundStuck(vehicle, &modelMin, &modelMax))
        {
            // There was nothing :p
        }
    }

    // Is valid driving
    if(!((isBike || vehicle->GetNumContactWheels() > 2) && ((vehicle->m_fMovingSpeed * (*ms_fTimeStep * fMagic)) > 0.01f)))
    {
        xdata->freeLineTimer = 0.0f;
        return;
    }

    // I can wait
    if(!(vehicle->m_AutoPilot.pTargetEntity == NULL && (!vehicle->vehicleFlags.bSirenOrAlarm || vehicle->m_nModelIndex == 423))) return;

    float speed = vehicle->m_vecMoveSpeed.Magnitude() * 180.0f;
    float distanceToCam = DistanceBetweenPoints(TheCamera->GetPosition(), vehicle->GetPosition());

    // Looks like it isn't stucked on the ground
    if (speed > 10.0f && distanceToCam < 30.0f) xdata->flags.bRunGroundStuckFix = false;

    // Disable exagerated driving style
    if (vehicle->m_AutoPilot.DrivingMode == DRIVING_STYLE_PLOUGH_THROUGH && !IsLawEnforcementVehicle(vehicle))
    {
        vehicle->m_AutoPilot.DrivingMode = DRIVING_STYLE_AVOID_CARS;
    }

    // Make use of driving style "6" for cars and bicycles
    if (vehicle->m_AutoPilot.DrivingMode <= DRIVING_STYLE_AVOID_CARS ||
        vehicle->m_AutoPilot.DrivingMode == DRIVING_STYLE_STOP_FOR_CARS_IGNORE_LIGHTS)
    {
        if (isBike)
        {
            if (subClass == VEHICLE_TYPE_BMX)
            {
                if (UseBikeLogicOnBicycles) vehicle->m_AutoPilot.DrivingMode = DRIVING_STYLE_AVOID_CARS_STOP_FOR_PEDS_OBEY_LIGHTS;
                if (BicyclesDontStopForRed) vehicle->m_AutoPilot.DrivingMode = DRIVING_STYLE_PLOUGH_THROUGH;
            }
        }
        else
        {
            if (UseBikeLogicOnCars)
            {
                if (HasCarStoppedBecauseOfLight(vehicle)) vehicle->m_AutoPilot.DrivingMode = DRIVING_STYLE_STOP_FOR_CARS;
                else vehicle->m_AutoPilot.DrivingMode = DRIVING_STYLE_AVOID_CARS_STOP_FOR_PEDS_OBEY_LIGHTS;
            }
        }
    }

    // Only in normal behaviour
    if (OnlyInNormal && vehicle->m_AutoPilot.DrivingMode != DRIVING_STYLE_STOP_FOR_CARS) return;

    // Check reverse gear
    if (vehicle->m_nCurrentGear <= 0) return;

    // Decrease speed turning
    float absSteerAngle = fabsf(vehicle->m_fSteerAngle);
    if (absSteerAngle > 0.001)
    {
        float turningDecrease = TurningSpeedDecrease * absSteerAngle;
        fNewCruiseSpeed = NormalizeCruiseSpeed(fNewCruiseSpeed - turningDecrease);
    }

    float speedFactor = speed / 40.0f;
    if (speedFactor > 1.0f) speedFactor = 1.0f;

    float dist = vehicle->m_vecMoveSpeed.Magnitude() * FrontMultDist;
    if (dist < 1.0f) dist = 1.0f;

    bool foundGround = false, forceObstacle = false;
    float newZ, steerBackOffset;
    CVector offsetA[3], coordA[3], coordB[3], offset;
    float steerOffset = vehicle->m_fSteerAngle * (-20.0f * speedFactor);

    float frontHeight = (modelMin.z * 0.4f);
    if (frontHeight > 1.0f) frontHeight = 1.0f;
    else if (frontHeight < -1.0f) frontHeight = -1.0f;

    float frontHeightBonus = 0.15f;
    if (isBike) frontHeightBonus = 0.5f;

    float heightDiffLimit = HeightDiffLimit;
    if (isBike) heightDiffLimit *= 2.0f;

    offset = { 0.0, modelMax.y, (frontHeight + frontHeightBonus) };
    coordA[0] = GetWorldCoordWithOffset(vehicle, offset);
    offset = { steerOffset, (modelMax.y + dist), ((frontHeight / 1.5f) + (frontHeightBonus * 2.0f)) };
    coordB[0] = GetWorldCoordWithOffset(vehicle, offset);
    newZ = FindGroundZFor3DCoord(coordB[0].x, coordB[0].y, coordB[0].z + CheckGroundHeight, &foundGround, NULL);

    if (fabsf((newZ - coordB[0].z)) > heightDiffLimit) forceObstacle = true; else coordB[0].z = newZ + FinalGroundHeight;

    if (!isBike)
    {
        // L
        steerBackOffset = (modelMax.y * vehicle->m_fSteerAngle);
        if (steerBackOffset < 0.0f) steerBackOffset = 0.0f;
        offsetA[1] = { modelMin.x, modelMax.y - steerBackOffset, frontHeight + frontHeightBonus };
        coordA[1] = GetWorldCoordWithOffset(vehicle, offsetA[1]);

        offset = { modelMin.x - (speed / SidesSpeedOffsetDiv) + steerOffset, (modelMax.y + dist), (frontHeight / 1.5f) };
        coordB[1] = GetWorldCoordWithOffset(vehicle, offset);
        newZ = FindGroundZFor3DCoord(coordB[1].x, coordB[1].y, coordB[1].z + CheckGroundHeight, &foundGround, NULL);
        if (fabsf((newZ - coordB[1].z)) > HeightDiffLimit) forceObstacle = true; else coordB[1].z = newZ + FinalGroundHeight;

        // R
        steerBackOffset = (modelMax.y * vehicle->m_fSteerAngle);
        if (steerBackOffset > 0.0f) steerBackOffset = 0.0f;
        offset = { modelMax.x, modelMax.y + steerBackOffset, frontHeight + frontHeightBonus };
        coordA[2] = GetWorldCoordWithOffset(vehicle, offset);

        offset = { modelMax.x + (speed / SidesSpeedOffsetDiv) + steerOffset, (modelMax.y + dist), (frontHeight / 1.5f) };
        coordB[2] = GetWorldCoordWithOffset(vehicle, offset);
        newZ = FindGroundZFor3DCoord(coordB[2].x, coordB[2].y, coordB[2].z + CheckGroundHeight, &foundGround, NULL);
        if (fabsf((newZ - coordB[2].z)) > HeightDiffLimit)
        {
            forceObstacle = true;
        }
        else
        {
            coordB[2].z = newZ + FinalGroundHeight;
        }
    }

    float obstacleDistFactor;
    bool bObstacle = false;
    *pIgnoreEntity = vehicle;
    CColPoint outColPoint;
    CEntity *outEntityC = NULL, *outEntityL = NULL, *outEntityR = NULL;
    if (forceObstacle)
    {
        obstacleDistFactor = 1.0f;
        goto label_force_obstacle;
    }

    if (isBike)
    {
        if (ProcessLineOfSight(coordA[0], coordB[0], outColPoint, outEntityC, 1, 1, 1, CheckObjects, 0, 0, 0, 0))
        {
            goto label_obstacle;
        }
        else
        {
            goto label_force_no_obstacle;
        }
    }

    if (ProcessLineOfSight(coordA[0], coordB[0], outColPoint, outEntityC, 1, 1, 1, CheckObjects, 0, 0, 0, 0) ||
        ProcessLineOfSight(coordA[1], coordB[1], outColPoint, outEntityL, 1, 1, 1, CheckObjects, 0, 0, 0, 0) ||
        ProcessLineOfSight(coordA[2], coordB[2], outColPoint, outEntityR, 1, 1, 1, CheckObjects, 0, 0, 0, 0))
    {
      label_obstacle:
        obstacleDistFactor = DistanceBetweenPoints(vehicle->GetPosition(), outColPoint.m_vecPoint) / 20.0f;
        if (obstacleDistFactor > 1.0f) obstacleDistFactor = 1.0f;
        obstacleDistFactor = 1.0f - obstacleDistFactor;

      label_force_obstacle:
        xdata->freeLineTimer -= ((obstacleDistFactor * speedFactor) * (*ms_fTimeStep / fMagic));
        if (xdata->freeLineTimer < 0.0f) xdata->freeLineTimer = 0.0f;

        fNewCruiseSpeed = NormalizeCruiseSpeed((float)(vehicle->m_AutoPilot.CruiseSpeed) - ((ObstacleSpeedDecrease * obstacleDistFactor) * (*ms_fTimeStep / fMagic)));
    }
    else
    {
      label_force_no_obstacle:
        if (false)
        {
            // Bumps part here?
            // UPD: Speed bumps mod
        }
        else
        {
            if (vehicle->m_fGasPedal >= 0.05f && !vehicle->vehicleFlags.bParking) // Accelerate
            {
                xdata->freeLineTimer += (Acceleration * (*ms_fTimeStep / fMagic));
                if (xdata->freeLineTimer > 1.0f) xdata->freeLineTimer = 1.0f;
                float freeLineSpeedIncrease = (CruiseMaxSpeed - CruiseSpeed) * xdata->freeLineTimer;
                fNewCruiseSpeed = NormalizeCruiseSpeed(fNewCruiseSpeed + freeLineSpeedIncrease);
            }
            else // I don't want to accelerate
            {
                xdata->freeLineTimer -= (DeAcceleration * (*ms_fTimeStep / fMagic));
                if (xdata->freeLineTimer < 0.0f) xdata->freeLineTimer = 0.0f;
            }
        }
    }

    // Set final cruise speed
    if (subClass == VEHICLE_TYPE_BMX)
    {
        fNewCruiseSpeed *= BicycleSpeedMult;
        if (fNewCruiseSpeed > BicycleCruiseMaxSpeed) fNewCruiseSpeed = BicycleCruiseMaxSpeed;
    }
    vehicle->m_AutoPilot.CruiseSpeed = (uint8_t)fNewCruiseSpeed;
}

// VehicleDestroy hook defensivo
DECL_HOOKv(VehicleDestroy, CVehicle* self)
{
    auto ext = GetExtData(self);
    if(ext) ext->Reset();
    VehicleDestroy(self);
}