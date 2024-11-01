/**
************************************************************
* @file         gizwits_protocol.c
* @brief        Corresponding gizwits_product.c header file (including product hardware and software version definition)
* @author       Gizwits
* @date         2017-07-19
* @version      V03030000
* @copyright    Gizwits
* 
* @note         机智云.只为智能硬件而生
*               Gizwits Smart Cloud  for Smart Products
*               链接|增值ֵ|开放|中立|安全|自有|自由|生态
*               www.gizwits.com
*
***********************************************************/
#include "ringBuffer.h"
#include "gizwits_product.h"
#include "dataPointTools.h"
//#include "hal_flash.h"
/** Protocol global variables **/
gizwitsProtocol_t gizwitsProtocol;
#ifdef OTA_ENABLE
mcuOTA_t	romUpdate;
#endif
/**@name The serial port receives the ring buffer implementation
* @{
*/
rb_t pRb;                                               ///< Ring buffer structure variable
static uint8_t rbBuf[RB_MAX_LEN];                       ///< Ring buffer data cache buffer


/**@} */

/**
* @brief Write data to the ring buffer
* @param [in] buf        : buf adress
* @param [in] len        : byte length
* @return   correct : Returns the length of the written data
            failure : -1
*/
int32_t gizPutData(uint8_t *buf, uint32_t len)
{
    int32_t count = 0;

    if(NULL == buf)
    {
        GIZWITS_LOG("ERR: gizPutData buf is empty \n");
        return -1;
    }

    count = rbWrite(&pRb, buf, len);
    if(count != len)
    {
        GIZWITS_LOG("ERR: Failed to rbWrite \n");
        return -1;
    }

    return count;
}



/**
* @brief Protocol header initialization
*
* @param [out] head         : Protocol header pointer
*
* @return 0， success; other， failure    
*/
static int8_t gizProtocolHeadInit(protocolHead_t *head)
{
    if(NULL == head)
    {
        GIZWITS_LOG("ERR: gizProtocolHeadInit head is empty \n");
        return -1;
    }

    memset((uint8_t *)head, 0, sizeof(protocolHead_t));
    head->head[0] = 0xFF;
    head->head[1] = 0xFF;

    return 0;
}

/**
* @brief Protocol ACK check processing function
*
* @param [in] data            : data adress
* @param [in] len             : data length
*
* @return 0， suceess; other， failure
*/
static int8_t gizProtocolWaitAck(uint8_t *gizdata, uint32_t len)
{
    if(NULL == gizdata)
    {
        GIZWITS_LOG("ERR: data is empty \n");
        return -1;
    }

    memset((uint8_t *)&gizwitsProtocol.waitAck, 0, sizeof(protocolWaitAck_t));
    memcpy((uint8_t *)gizwitsProtocol.waitAck.buf, gizdata, len);
    gizwitsProtocol.waitAck.dataLen = (uint16_t)len;
    
    gizwitsProtocol.waitAck.flag = 1;
    gizwitsProtocol.waitAck.sendTime = gizGetTimerCount();

    return 0;
}
/**
* @brief Calculates the byte size occupied by the bit
*
* @param [in] aFlag: P0 flag data
*
* @return: byte size
*/
uint32_t ICACHE_FLASH_ATTR calculateDynamicBitPartLen(dataPointFlags_t *aFlag)
{
    uint32_t bitFieldBitLen = 0,bytetmpLen= 0;
    /* Processing only writable bool Enum type data */
    if(0x01 == aFlag->flagModel)
    {
        bitFieldBitLen += Model_LEN;
    }

    if(0 == bitFieldBitLen)
    {
        bytetmpLen = 0;
    }
    else
    {
        if(0 == bitFieldBitLen%8)
        {
            bytetmpLen = bitFieldBitLen/8;
        }
        else
        {
            bytetmpLen = bitFieldBitLen/8 + 1;
        }
    }
    return bytetmpLen;
}

/**
* @brief generates "controlled events" according to protocol

* @param [in] issuedData: Controlled data
* @param [out] info: event queue
* @param [out] dataPoints: data point data
* @return 0, the implementation of success, non-0, failed
*/
static int8_t ICACHE_FLASH_ATTR gizDataPoint2Event(uint8_t *issuedData, eventInfo_t *info, dataPoint_t *dataPoints)
{
    uint32_t bitFieldByteLen= 0;//Bit segment length
    uint32_t bitFieldOffset = 0;//Bit position offset
    uint32_t byteFieldOffset = 0;//Byte segment offset

    gizwitsElongateP0Form_t elongateP0FormTmp;
    gizMemset((uint8_t *)&elongateP0FormTmp,0,sizeof(gizwitsElongateP0Form_t));
    gizMemcpy((uint8_t *)&elongateP0FormTmp.devDatapointFlag,issuedData,DATAPOINT_FLAG_LEN);

    if((NULL == issuedData) || (NULL == info) ||(NULL == dataPoints))
    {
        GIZWITS_LOG("gizDataPoint2Event Error , Illegal Param\n");
        return -1;
    }
	
    /** Greater than 1 byte to do bit conversion **/
    if(DATAPOINT_FLAG_LEN > 1)
    {
        if(-1 == gizByteOrderExchange((uint8_t *)&elongateP0FormTmp.devDatapointFlag,DATAPOINT_FLAG_LEN))
        {
            GIZWITS_LOG("gizByteOrderExchange Error\n");
            return -1;
        }
    }
    /* Calculates the byte length occupied by the segment */
    bitFieldByteLen = calculateDynamicBitPartLen(&elongateP0FormTmp.devDatapointFlag);
    byteFieldOffset += bitFieldByteLen + DATAPOINT_FLAG_LEN;//Value segment byte offset

    if(0x01 == elongateP0FormTmp.devDatapointFlag.flagModel)
    {
        info->event[info->num] = EVENT_Model;
        info->num++;
        dataPoints->valueModel = gizVarlenDecompressionValue(bitFieldOffset,Model_LEN,(uint8_t *)&issuedData[DATAPOINT_FLAG_LEN],bitFieldByteLen);
        bitFieldOffset += Model_LEN;
    }


    if(0x01 == elongateP0FormTmp.devDatapointFlag.flagLamp_brightness)
    {
        info->event[info->num] = EVENT_Lamp_brightness;
        info->num++;
        gizMemcpy((uint8_t *)&elongateP0FormTmp.devStatus.valueLamp_brightness,(uint8_t *)&issuedData[byteFieldOffset],Lamp_brightness_LEN);
        byteFieldOffset += Lamp_brightness_LEN;
        dataPoints->valueLamp_brightness = gizX2Y(Lamp_brightness_RATIO,  Lamp_brightness_ADDITION, elongateP0FormTmp.devStatus.valueLamp_brightness); 
    }

    if(0x01 == elongateP0FormTmp.devDatapointFlag.flagDistance_threshold)
    {
        info->event[info->num] = EVENT_Distance_threshold;
        info->num++;
        gizMemcpy((uint8_t *)&elongateP0FormTmp.devStatus.valueDistance_threshold,(uint8_t *)&issuedData[byteFieldOffset],Distance_threshold_LEN);
        byteFieldOffset += Distance_threshold_LEN;
        dataPoints->valueDistance_threshold = gizX2Y(Distance_threshold_RATIO,  Distance_threshold_ADDITION, elongateP0FormTmp.devStatus.valueDistance_threshold); 
    }

    if(0x01 == elongateP0FormTmp.devDatapointFlag.flagIllumination_threshold)
    {
        info->event[info->num] = EVENT_Illumination_threshold;
        info->num++;
        gizMemcpy((uint8_t *)&elongateP0FormTmp.devStatus.valueIllumination_threshold,(uint8_t *)&issuedData[byteFieldOffset],Illumination_threshold_LEN);
        byteFieldOffset += Illumination_threshold_LEN;
        dataPoints->valueIllumination_threshold = gizX2Y(Illumination_threshold_RATIO,  Illumination_threshold_ADDITION, exchangeBytes(elongateP0FormTmp.devStatus.valueIllumination_threshold));
    }

    return 0;
}


/**
* @brief contrasts the current data with the last data
*
* @param [in] cur: current data point data
* @param [in] last: last data point data
*
* @return: 0, no change in data; 1, data changes
*/
static int8_t ICACHE_FLASH_ATTR gizCheckReport(dataPoint_t *cur, dataPoint_t *last)
{
    int8_t ret = 0;
    static uint32_t lastReportTime = 0;
    uint32_t currentTime = 0;

    if((NULL == cur) || (NULL == last))
    {
        GIZWITS_LOG("gizCheckReport Error , Illegal Param\n");
        return -1;
    }
    currentTime = gizGetTimerCount();

    if(last->valueModel != cur->valueModel)
    {
        GIZWITS_LOG("valueModel Changed\n");
        gizwitsProtocol.waitReportDatapointFlag.flagModel = 1;
        ret = 1;
    }
    if(last->valuebody_recognition != cur->valuebody_recognition)
    {
        GIZWITS_LOG("valuebody_recognition Changed\n");
        gizwitsProtocol.waitReportDatapointFlag.flagbody_recognition = 1;
        ret = 1;
    }
    if(last->valueLamp_brightness != cur->valueLamp_brightness)
    {
        GIZWITS_LOG("valueLamp_brightness Changed\n");
        gizwitsProtocol.waitReportDatapointFlag.flagLamp_brightness = 1;
        ret = 1;
    }
    if(last->valueDistance_threshold != cur->valueDistance_threshold)
    {
        GIZWITS_LOG("valueDistance_threshold Changed\n");
        gizwitsProtocol.waitReportDatapointFlag.flagDistance_threshold = 1;
        ret = 1;
    }
    if(last->valueIllumination_threshold != cur->valueIllumination_threshold)
    {
        GIZWITS_LOG("valueIllumination_threshold Changed\n");
        gizwitsProtocol.waitReportDatapointFlag.flagIllumination_threshold = 1;
        ret = 1;
    }

    if(last->valueTemperature != cur->valueTemperature)
    {
        if(currentTime - lastReportTime >= REPORT_TIME_MAX)
        {
            GIZWITS_LOG("valueTemperature Changed\n");
            gizwitsProtocol.waitReportDatapointFlag.flagTemperature = 1;
            ret = 1;
        }
    }
    if(last->valueHumidity != cur->valueHumidity)
    {
        if(currentTime - lastReportTime >= REPORT_TIME_MAX)
        {
            GIZWITS_LOG("valueHumidity Changed\n");
            gizwitsProtocol.waitReportDatapointFlag.flagHumidity = 1;
            ret = 1;
        }
    }
    if(last->valuedistance != cur->valuedistance)
    {
        if(currentTime - lastReportTime >= REPORT_TIME_MAX)
        {
            GIZWITS_LOG("valuedistance Changed\n");
            gizwitsProtocol.waitReportDatapointFlag.flagdistance = 1;
            ret = 1;
        }
    }
    if(last->valueLux != cur->valueLux)
    {
        if(currentTime - lastReportTime >= REPORT_TIME_MAX)
        {
            GIZWITS_LOG("valueLux Changed\n");
            gizwitsProtocol.waitReportDatapointFlag.flagLux = 1;
            ret = 1;
        }
    }

    if(1 == ret)
    {
        lastReportTime = gizGetTimerCount();
    }

    return ret;
}

/**
* @brief User data point data is converted to wit the cloud to report data point data
*
* @param [in] dataPoints: user data point data address
* @param [out] devStatusPtr: wit the cloud data point data address
*
* @return 0, the correct return; -1, the error returned
*/
static int8_t ICACHE_FLASH_ATTR gizDataPoints2ReportData(dataPoint_t *dataPoints , uint8_t *outData,uint32_t *outDataLen)
{
    uint32_t bitFieldByteLen= 0;//Bit byte size
    uint32_t bitFieldOffset = 0;//Bit offset
    uint32_t byteFieldOffset = 0;//Byte offset
	devStatus_t devStatusTmp;//Temporary device data point variable
    uint8_t allDatapointByteBuf[sizeof(gizwitsElongateP0Form_t)];//Open up the largest data point space
    gizMemset(allDatapointByteBuf,0,sizeof(gizwitsElongateP0Form_t));

    gizMemcpy(allDatapointByteBuf,(uint8_t *)&gizwitsProtocol.waitReportDatapointFlag,DATAPOINT_FLAG_LEN);
    if(DATAPOINT_FLAG_LEN > 1)
    {
        gizByteOrderExchange(allDatapointByteBuf,DATAPOINT_FLAG_LEN);
    }
    byteFieldOffset += DATAPOINT_FLAG_LEN;//First offset the flag size of the location
        
    if((NULL == dataPoints) || (NULL == outData))
    {
        GIZWITS_LOG("gizDataPoints2ReportData Error , Illegal Param\n");
        return -1;
    }

    /*** Fill the bit ***/
    if(gizwitsProtocol.waitReportDatapointFlag.flagModel)
    {
        if(dataPoints->valueModel >= Model_VALUE_MAX)
        {
            GIZWITS_LOG("[ERROR] valueModel Error , Illegal Overstep\n");
            return -1;
        }
        else
        {
                gizVarlenCompressValue(bitFieldOffset,Model_LEN,(uint8_t *)&allDatapointByteBuf[byteFieldOffset],dataPoints->valueModel);
                bitFieldOffset += Model_LEN;
        }
    }
    if(gizwitsProtocol.waitReportDatapointFlag.flagbody_recognition)
    {
        if(dataPoints->valuebody_recognition >= body_recognition_VALUE_MAX)
        {
            GIZWITS_LOG("[ERROR] valuebody_recognition Error , Illegal Overstep\n");
            return -1;
        }
        else
        {
                gizVarlenCompressValue(bitFieldOffset,body_recognition_LEN,(uint8_t *)&allDatapointByteBuf[byteFieldOffset],dataPoints->valuebody_recognition);
                bitFieldOffset += body_recognition_LEN;
        }
    }

    /* The bit segment is assembled and the offset of the value segment is calculated */
    if(0 == bitFieldOffset)
    {
        bitFieldByteLen = 0;
    }
    else
    {
        if(0 == bitFieldOffset%8)
        {
            bitFieldByteLen = bitFieldOffset/8;
        }
        else
        {
            bitFieldByteLen = bitFieldOffset/8 + 1;
        }
    }
    /** Bitwise byte order conversion **/
    if(bitFieldByteLen > 1)
    {
        gizByteOrderExchange((uint8_t *)&allDatapointByteBuf[byteFieldOffset],bitFieldByteLen);
    }
    
    byteFieldOffset += bitFieldByteLen;//Offset the number of bytes occupied by the bit segment

    /*** Handle the value segment ***/

    if(gizwitsProtocol.waitReportDatapointFlag.flagTemperature)
    {
        devStatusTmp.valueTemperature = gizY2X(Temperature_RATIO,  Temperature_ADDITION, dataPoints->valueTemperature); 
        gizMemcpy((uint8_t *)&allDatapointByteBuf[byteFieldOffset],(uint8_t *)&devStatusTmp.valueTemperature,Temperature_LEN);
        byteFieldOffset += Temperature_LEN;
    }
    if(gizwitsProtocol.waitReportDatapointFlag.flagHumidity)
    {
        devStatusTmp.valueHumidity = gizY2X(Humidity_RATIO,  Humidity_ADDITION, dataPoints->valueHumidity); 
        gizMemcpy((uint8_t *)&allDatapointByteBuf[byteFieldOffset],(uint8_t *)&devStatusTmp.valueHumidity,Humidity_LEN);
        byteFieldOffset += Humidity_LEN;
    }
    if(gizwitsProtocol.waitReportDatapointFlag.flagdistance)
    {
        devStatusTmp.valuedistance = gizY2X(distance_RATIO,  distance_ADDITION, dataPoints->valuedistance); 
        gizMemcpy((uint8_t *)&allDatapointByteBuf[byteFieldOffset],(uint8_t *)&devStatusTmp.valuedistance,distance_LEN);
        byteFieldOffset += distance_LEN;
    }
    if(gizwitsProtocol.waitReportDatapointFlag.flagLamp_brightness)
    {
        devStatusTmp.valueLamp_brightness = gizY2X(Lamp_brightness_RATIO,  Lamp_brightness_ADDITION, dataPoints->valueLamp_brightness); 
        gizMemcpy((uint8_t *)&allDatapointByteBuf[byteFieldOffset],(uint8_t *)&devStatusTmp.valueLamp_brightness,Lamp_brightness_LEN);
        byteFieldOffset += Lamp_brightness_LEN;
    }
    if(gizwitsProtocol.waitReportDatapointFlag.flagDistance_threshold)
    {
        devStatusTmp.valueDistance_threshold = gizY2X(Distance_threshold_RATIO,  Distance_threshold_ADDITION, dataPoints->valueDistance_threshold); 
        gizMemcpy((uint8_t *)&allDatapointByteBuf[byteFieldOffset],(uint8_t *)&devStatusTmp.valueDistance_threshold,Distance_threshold_LEN);
        byteFieldOffset += Distance_threshold_LEN;
    }

    if(gizwitsProtocol.waitReportDatapointFlag.flagLux)
    {
        devStatusTmp.valueLux = exchangeBytes(gizY2X(Lux_RATIO,  Lux_ADDITION, dataPoints->valueLux)); 
        gizMemcpy((uint8_t *)&allDatapointByteBuf[byteFieldOffset],(uint8_t *)&devStatusTmp.valueLux,Lux_LEN);
        byteFieldOffset += Lux_LEN;
    }
    if(gizwitsProtocol.waitReportDatapointFlag.flagIllumination_threshold)
    {
        devStatusTmp.valueIllumination_threshold = exchangeBytes(gizY2X(Illumination_threshold_RATIO,  Illumination_threshold_ADDITION, dataPoints->valueIllumination_threshold)); 
        gizMemcpy((uint8_t *)&allDatapointByteBuf[byteFieldOffset],(uint8_t *)&devStatusTmp.valueIllumination_threshold,Illumination_threshold_LEN);
        byteFieldOffset += Illumination_threshold_LEN;
    }




    gizMemset((uint8_t *)&gizwitsProtocol.waitReportDatapointFlag,0,DATAPOINT_FLAG_LEN);//Clear the flag
    *outDataLen = byteFieldOffset;
    gizMemcpy(outData,allDatapointByteBuf,*outDataLen);
    return 0;
}


/**
* @brief This function is called by the Gagent module to receive the relevant protocol data from the cloud or APP
* @param [in] inData The protocol data entered
* @param [in] inLen Enter the length of the data
* @param [out] outData The output of the protocol data
* @param [out] outLen The length of the output data
* @return 0, the implementation of success, non-0, failed
*/
static int8_t gizProtocolIssuedProcess(char *did, uint8_t *inData, uint32_t inLen, uint8_t *outData, uint32_t *outLen)
{
    uint8_t issuedAction = inData[0];

    if((NULL == inData)||(NULL == outData)||(NULL == outLen))
    {
        GIZWITS_LOG("gizProtocolIssuedProcess Error , Illegal Param\n");
        return -1;
    }

    if(NULL == did)
    {
    memset((uint8_t *)&gizwitsProtocol.issuedProcessEvent, 0, sizeof(eventInfo_t));
    switch(issuedAction)
    {
        case ACTION_CONTROL_DEVICE:
            GIZWITS_LOG("ACTION_ELONGATE_CONTROL_DEVICE \n");
            gizDataPoint2Event(&inData[1], &gizwitsProtocol.issuedProcessEvent,&gizwitsProtocol.gizCurrentDataPoint);
            gizwitsProtocol.issuedFlag = ACTION_CONTROL_TYPE;
            outData = NULL;
            *outLen = 0;
            break;
        
        case ACTION_READ_DEV_STATUS:
            GIZWITS_LOG("ACTION_ELONGATE_READ_DEV_STATUS \n");          
            memcpy((uint8_t *)&gizwitsProtocol.waitReportDatapointFlag,&inData[1],DATAPOINT_FLAG_LEN);//拷贝查询FLAG               
            if(DATAPOINT_FLAG_LEN > 1)
            {
                    gizByteOrderExchange((uint8_t *)&gizwitsProtocol.waitReportDatapointFlag,DATAPOINT_FLAG_LEN);
            }
                        
            if(0 == gizDataPoints2ReportData(&gizwitsProtocol.gizLastDataPoint,gizwitsProtocol.reportData,(uint32_t *)&gizwitsProtocol.reportDataLen))
            {  
                memcpy(outData+1, (uint8_t *)&gizwitsProtocol.reportData, gizwitsProtocol.reportDataLen);
                outData[0] = ACTION_READ_DEV_STATUS_ACK;
                *outLen = DATAPOINT_FLAG_LEN + sizeof(gizwitsReport_t)+1;
            }
            else
            {
                return -1;
            }
            break;
        case ACTION_W2D_TRANSPARENT_DATA:
            memcpy(gizwitsProtocol.transparentBuff, &inData[1], inLen-1);
            gizwitsProtocol.transparentLen = inLen - 1;
            
            gizwitsProtocol.issuedProcessEvent.event[gizwitsProtocol.issuedProcessEvent.num] = TRANSPARENT_DATA;
            gizwitsProtocol.issuedProcessEvent.num++;
            gizwitsProtocol.issuedFlag = ACTION_W2D_TRANSPARENT_TYPE;
            outData = NULL;
            *outLen = 0;
            break;
        
        default:
            break;
        }
    }

    return 0;
}

/**
* @brief The protocol sends data back , P0 ACK
*
* @param [in] head                  : Protocol head pointer
* @param [in] data                  : Payload data 
* @param [in] len                   : Payload data length
* @param [in] proFlag               : DID flag ,1 for Virtual sub device did ,0 for single product or gateway 
*
* @return : 0,Ack success;
*           -1，Input Param Illegal
*           -2，Serial send faild
*/
static int32_t gizProtocolIssuedDataAck(protocolHead_t *head, uint8_t *gizdata, uint32_t len, uint8_t proFlag)
{
    int32_t ret = 0;
    uint8_t tx_buf[RB_MAX_LEN];
    uint32_t offset = 0;
    uint8_t sDidLen = 0;
    uint16_t data_len = 0;
	uint8_t *pTxBuf = tx_buf;
    if(NULL == gizdata)
    {
        GIZWITS_LOG("[ERR]  data Is Null \n");
        return -1;
    }
    

    if(0x1 == proFlag)
    {
        sDidLen = *((uint8_t *)head + sizeof(protocolHead_t));
        data_len = 5 + 1 + sDidLen + len;   
    }
    else
    {
        data_len = 5 + len;
    }
    GIZWITS_LOG("len = %d , sDidLen = %d ,data_len = %d\n", len,sDidLen,data_len);
    *pTxBuf ++= 0xFF;
    *pTxBuf ++= 0xFF;
    *pTxBuf ++= (uint8_t)(data_len>>8);//len
    *pTxBuf ++= (uint8_t)(data_len);
    *pTxBuf ++= head->cmd + 1;
    *pTxBuf ++= head->sn;
    *pTxBuf ++= 0x00;
    *pTxBuf ++= proFlag;//flag
    offset = 8;
    if(0x1 == proFlag)
    {
        *pTxBuf ++= sDidLen;//didlen
        offset += 1;
        memcpy(&tx_buf[offset],(uint8_t *)head+sizeof(protocolHead_t)+1,sDidLen);
        offset += sDidLen;
        pTxBuf += sDidLen;

    }
    if(0 != len)
    {
        memcpy(&tx_buf[offset],gizdata,len);
    }
    tx_buf[data_len + 4 - 1 ] = gizProtocolSum( tx_buf , (data_len+4));

    ret = uartWrite(tx_buf, data_len+4);
    if(ret < 0)
    {
        GIZWITS_LOG("uart write error %d \n", ret);
        return -2;
    }

    return 0;
}
/**
* @brief Report data interface
*
* @param [in] action            : PO action
* @param [in] data              : Payload data
* @param [in] len               : Payload data length
*
* @return : 0,Ack success;
*           -1，Input Param Illegal
*           -2，Serial send faild
*/
static int32_t gizReportData(uint8_t action, uint8_t *gizdata, uint32_t len)
{
    int32_t ret = 0;
    uint8_t tx_buf[MAX_PACKAGE_LEN];
	uint8_t *pTxBuf = tx_buf;
    uint16_t data_len = 6 + len;
    if(NULL == gizdata)
    {
        GIZWITS_LOG("gizReportData Error , Illegal Param\n");
        return -1;
    }

    *pTxBuf ++= 0xFF;
    *pTxBuf ++= 0xFF;
    *pTxBuf ++= (uint8_t)(data_len>>8);
    *pTxBuf ++= (uint8_t)(data_len);
    *pTxBuf ++= CMD_REPORT_P0;
    *pTxBuf ++= gizwitsProtocol.sn++;
    *pTxBuf ++= 0x00;
    *pTxBuf ++= 0x00;
    *pTxBuf ++= action;

    memcpy(&tx_buf[9],gizdata,len);
    
    tx_buf[data_len + 4 - 1 ] = gizProtocolSum( tx_buf , (data_len+4));
    
    ret = uartWrite(tx_buf, data_len+4);
    if(ret < 0)
    {
        GIZWITS_LOG("ERR: uart write error %d \n", ret);
        return -2;
    }

    gizProtocolWaitAck(tx_buf, data_len+4);

    return 0;
}/**
* @brief Datapoints reporting mechanism
*
* 1. Changes are reported immediately

* 2. Data timing report , 600000 Millisecond
* 
*@param [in] currentData       : Current datapoints value
* @return : NULL
*/
static void gizDevReportPolicy(dataPoint_t *currentData)
{
    static uint32_t lastRepTime = 0;
    uint32_t timeNow = gizGetTimerCount();
    uint8_t *waitReportDataPtr = NULL;

    if((1 == gizCheckReport(currentData, (dataPoint_t *)&gizwitsProtocol.gizLastDataPoint)))
    {
        GIZWITS_LOG("changed, report data\n");
        if(0 == gizDataPoints2ReportData(currentData,gizwitsProtocol.reportData,(uint32_t *)&gizwitsProtocol.reportDataLen))
        {
            gizReportData(ACTION_REPORT_DEV_STATUS, gizwitsProtocol.reportData, gizwitsProtocol.reportDataLen);        }
        memcpy((uint8_t *)&gizwitsProtocol.gizLastDataPoint, (uint8_t *)currentData, sizeof(dataPoint_t));
    }

    if((0 == (timeNow % (600000))) && (lastRepTime != timeNow))
    {
        GIZWITS_LOG("Info: 600S report data\n");
        memset((uint8_t *)&gizwitsProtocol.waitReportDatapointFlag,0xFF,DATAPOINT_FLAG_LEN);
        if(0 == gizDataPoints2ReportData(currentData,gizwitsProtocol.reportData,(uint32_t *)&gizwitsProtocol.reportDataLen))
        {
            gizReportData(ACTION_REPORT_DEV_STATUS, gizwitsProtocol.reportData, gizwitsProtocol.reportDataLen);        }
        memcpy((uint8_t *)&gizwitsProtocol.gizLastDataPoint, (uint8_t *)currentData, sizeof(dataPoint_t));

        lastRepTime = timeNow;
    }
    free(waitReportDataPtr);
}

/**
* @brief Get a packet of data from the ring buffer
*
* @param [in]  rb                  : Input data address
* @param [out] data                : Output data address
* @param [out] len                 : Output data length
*
* @return : 0,Return correct ;-1，Return failure;-2，Data check failure
*/
static int8_t gizProtocolGetOnePacket(rb_t *rb, uint8_t *gizdata, uint16_t *len)
{
    int32_t ret = 0;
    uint8_t sum = 0;
    int32_t i = 0;
    uint8_t tmpData;
    uint16_t tmpLen = 0;
    uint16_t tmpCount = 0;
    static uint8_t protocolFlag = 0;
    static uint16_t protocolCount = 0;
    static uint8_t lastData = 0;
    static uint8_t debugCount = 0;
    uint8_t *protocolBuff = gizdata;
    protocolHead_t *head = NULL;

    if((NULL == rb) || (NULL == gizdata) ||(NULL == len))
    {
        GIZWITS_LOG("gizProtocolGetOnePacket Error , Illegal Param\n");
        return -1;
    }

    tmpLen = rbCanRead(rb);
    if(0 == tmpLen)
    {
        return -1;
    }

    for(i=0; i<tmpLen; i++)
    {
        ret = rbRead(rb, &tmpData, 1);
        if(0 != ret)
        {
            if((0xFF == lastData) && (0xFF == tmpData))
            {
                if(0 == protocolFlag)
                {
                    protocolBuff[0] = 0xFF;
                    protocolBuff[1] = 0xFF;
                    protocolCount = 2;
                    protocolFlag = 1;
                }
                else
                {
                    if((protocolCount > 4) && (protocolCount != tmpCount))
                    {
                        protocolBuff[0] = 0xFF;
                        protocolBuff[1] = 0xFF;
                        protocolCount = 2;
                    }
                }
            }
            else if((0xFF == lastData) && (0x55 == tmpData))
            {
            }
            else
            {
                if(1 == protocolFlag)
                {
                    protocolBuff[protocolCount] = tmpData;
                    protocolCount++;

                    if(protocolCount > 4)
                    {
                        head = (protocolHead_t *)protocolBuff;
                        tmpCount = exchangeBytes(head->len)+4;
                        if (tmpCount >= MAX_PACKAGE_LEN || tmpCount <= 4)
                        {
                            protocolFlag = 0;
                            protocolCount = 0;
                            GIZWITS_LOG("ERR:the data length is too long or too small, will abandon \n");
                        }
                        if(protocolCount == tmpCount)
                        {
                            break;
                        }
                    }
                    
                }
            }

            lastData = tmpData;
            debugCount++;
        }
    }

    if((protocolCount > 4) && (protocolCount == tmpCount))
    {
        sum = gizProtocolSum(protocolBuff, protocolCount);

        if(protocolBuff[protocolCount-1] == sum)
        {
            *len = tmpCount;
            protocolFlag = 0;

            protocolCount = 0;
            debugCount = 0;
            lastData = 0;

            return 0;
        }
        else
        {
            protocolFlag = 0;
            protocolCount = 0;
            return -2;
        }
    }

    return 1;
}



/**
* @brief Protocol data resend

* The protocol data resend when check timeout and meet the resend limiting

* @param none    
*
* @return none
*/
static void gizProtocolResendData(void)
{
    int32_t ret = 0;

    if(0 == gizwitsProtocol.waitAck.flag)
    {
        return;
    }

    GIZWITS_LOG("Warning: timeout, resend data \n");
    
    ret = uartWrite(gizwitsProtocol.waitAck.buf, gizwitsProtocol.waitAck.dataLen);
    if(ret != gizwitsProtocol.waitAck.dataLen)
    {
        GIZWITS_LOG("ERR: resend data error\n");
    }

    gizwitsProtocol.waitAck.sendTime = gizGetTimerCount();
}

/**
* @brief Clear the ACK protocol message
*
* @param [in] head : Protocol header address
*
* @return 0， success; other， failure
*/
static int8_t gizProtocolWaitAckCheck(protocolHead_t *head)
{
    protocolHead_t *waitAckHead = (protocolHead_t *)gizwitsProtocol.waitAck.buf;

    if(NULL == head)
    {
        GIZWITS_LOG("ERR: data is empty \n");
        return -1;
    }

    if(waitAckHead->cmd+1 == head->cmd)
    {
        memset((uint8_t *)&gizwitsProtocol.waitAck, 0, sizeof(protocolWaitAck_t));
    }

    return 0;
}

/**
* @brief Send general protocol message data
* 
* @param [in] head              : Protocol header address
*
* @return : Return effective data length;-1，return failure
*/
static int32_t gizProtocolCommonAck(protocolHead_t *head)
{
    int32_t ret = 0;
    protocolCommon_t ack;

    if(NULL == head)
    {
        GIZWITS_LOG("ERR: gizProtocolCommonAck data is empty \n");
        return -1;
    }
    memcpy((uint8_t *)&ack, (uint8_t *)head, sizeof(protocolHead_t));
    ack.head.cmd = ack.head.cmd+1;
    ack.head.len = exchangeBytes(sizeof(protocolCommon_t)-4);
    ack.sum = gizProtocolSum((uint8_t *)&ack, sizeof(protocolCommon_t));

    ret = uartWrite((uint8_t *)&ack, sizeof(protocolCommon_t));
    if(ret < 0)
    {
        GIZWITS_LOG("ERR: uart write error %d \n", ret);
    }

    return ret;
}

/**
* @brief ACK processing function

* Time-out 200ms no ACK resend，resend two times at most

* @param none 
*
* @return none
*/
static void gizProtocolAckHandle(void)
{
    if(1 == gizwitsProtocol.waitAck.flag)
    {
        if(SEND_MAX_NUM > gizwitsProtocol.waitAck.num)
        {
            // Time-out no ACK resend
            if(SEND_MAX_TIME < (gizGetTimerCount() - gizwitsProtocol.waitAck.sendTime))
            {
                GIZWITS_LOG("Warning:gizProtocolResendData %d %d %d\n", gizGetTimerCount(), gizwitsProtocol.waitAck.sendTime, gizwitsProtocol.waitAck.num);
                gizProtocolResendData();
                gizwitsProtocol.waitAck.num++;
            }
        }
        else
        {
            memset((uint8_t *)&gizwitsProtocol.waitAck, 0, sizeof(protocolWaitAck_t));
        }
    }
}

/**
* @brief Protocol 4.1 WiFi module requests device information
*
* @param[in] head : Protocol header address
*
* @return Return effective data length;-1，return failure
*/
static int32_t gizProtocolGetDeviceInfo(protocolHead_t * head)
{
    int32_t ret = 0;
    protocolDeviceInfo_t deviceInfo;

    if(NULL == head)
    {
        GIZWITS_LOG("gizProtocolGetDeviceInfo Error , Illegal Param\n");
        return -1;
    }

    gizProtocolHeadInit((protocolHead_t *)&deviceInfo);
    deviceInfo.head.cmd = ACK_GET_DEVICE_INFO;
    deviceInfo.head.sn = head->sn;
    memcpy((uint8_t *)deviceInfo.protocolVer, protocol_VERSION, 8);
    memcpy((uint8_t *)deviceInfo.p0Ver, P0_VERSION, 8);
    memcpy((uint8_t *)deviceInfo.softVer, SOFTWARE_VERSION, 8);
    memcpy((uint8_t *)deviceInfo.hardVer, HARDWARE_VERSION, 8);
    memcpy((uint8_t *)deviceInfo.productKey, PRODUCT_KEY, strlen(PRODUCT_KEY));
    memcpy((uint8_t *)deviceInfo.productSecret, PRODUCT_SECRET, strlen(PRODUCT_SECRET));
    memset((uint8_t *)deviceInfo.devAttr, 0, 8);
    deviceInfo.devAttr[7] |= DEV_IS_GATEWAY<<0;
    deviceInfo.devAttr[7] |= (0x01<<1);    
    deviceInfo.ninableTime = exchangeBytes(NINABLETIME);
#ifdef DATA_CONFIG_ENABLE
    deviceInfo.DataLen = GIZ_DATA_LEN;
    sprintf(deviceInfo.Data,"apName=%s&apPwd=%s&cfgMode=%s",AP_NAME,AP_PWD,CFG_MODE); 
#endif
    deviceInfo.head.len = exchangeBytes(sizeof(protocolDeviceInfo_t)-4);
    deviceInfo.sum = gizProtocolSum((uint8_t *)&deviceInfo, sizeof(protocolDeviceInfo_t));

    ret = uartWrite((uint8_t *)&deviceInfo, sizeof(protocolDeviceInfo_t));
    if(ret < 0)
    {
        GIZWITS_LOG("ERR: uart write error %d \n", ret);
    }
    
    return ret;
}

/**
* @brief Protocol 4.7 Handling of illegal message notification

* @param[in] head  : Protocol header address
* @param[in] errno : Illegal message notification type
* @return 0， success; other， failure
*/
static int32_t gizProtocolErrorCmd(protocolHead_t *head,errorPacketsType_t errno)
{
    int32_t ret = 0;
    protocolErrorType_t errorType;

    if(NULL == head)
    {
        GIZWITS_LOG("gizProtocolErrorCmd Error , Illegal Param\n");
        return -1;
    }
    gizProtocolHeadInit((protocolHead_t *)&errorType);
    errorType.head.cmd = ACK_ERROR_PACKAGE;
    errorType.head.sn = head->sn;
    
    errorType.head.len = exchangeBytes(sizeof(protocolErrorType_t)-4);
    errorType.error = errno;
    errorType.sum = gizProtocolSum((uint8_t *)&errorType, sizeof(protocolErrorType_t));
    
    ret = uartWrite((uint8_t *)&errorType, sizeof(protocolErrorType_t));
    if(ret < 0)
    {
        GIZWITS_LOG("ERR: uart write error %d \n", ret);
    }

    return ret;
}

/**
* @brief Protocol 4.13 Get and process network time
*
* @param [in] head : Protocol header address
*
* @return 0， success; other， failure
*/
static int8_t gizProtocolNTP(protocolHead_t *head)
{  
    protocolUTT_t *UTTInfo = (protocolUTT_t *)head;
    
    if(NULL == head)
    {
        GIZWITS_LOG("ERR: NTP is empty \n");
        return -1;
    }
    
    memcpy((uint8_t *)&gizwitsProtocol.TimeNTP.year,(uint8_t *)UTTInfo->time, 7);
    memcpy((uint8_t *)&gizwitsProtocol.TimeNTP.ntp,(uint8_t *)UTTInfo->ntp_time, 4);
    gizwitsProtocol.TimeNTP.year = exchangeBytes(gizwitsProtocol.TimeNTP.year);
    gizwitsProtocol.TimeNTP.ntp =exchangeWord(gizwitsProtocol.TimeNTP.ntp);

    gizwitsProtocol.NTPEvent.event[gizwitsProtocol.NTPEvent.num] = WIFI_NTP;
    gizwitsProtocol.NTPEvent.num++;
    
    gizwitsProtocol.issuedFlag = GET_NTP_TYPE;
    
    
    return 0;
}

/**
* @brief Protocol 4.4 Device MCU restarts function

* @param none
* @return none
*/
static void gizProtocolReboot(void)
{
    uint32_t timeDelay = gizGetTimerCount();
    
    /*Wait 600ms*/
    while((gizGetTimerCount() - timeDelay) <= 600);
    mcuRestart();
}

/**
* @brief Protocol 4.5 :The WiFi module informs the device MCU of working status about the WiFi module

* @param[in] status WiFi module working status
* @return none
*/
static int8_t gizProtocolModuleStatus(protocolWifiStatus_t *status)
{
    static wifiStatus_t lastStatus;

    if(NULL == status)
    {
        GIZWITS_LOG("gizProtocolModuleStatus Error , Illegal Param\n");
        return -1;
    }

    status->ststus.value = exchangeBytes(status->ststus.value);
   
    //OnBoarding mode status
    if(lastStatus.types.onboarding != status->ststus.types.onboarding)
    {
        if(1 == status->ststus.types.onboarding)
        {
            if(1 == status->ststus.types.softap)
            {
                gizwitsProtocol.wifiStatusEvent.event[gizwitsProtocol.wifiStatusEvent.num] = WIFI_SOFTAP;
                gizwitsProtocol.wifiStatusEvent.num++;
                GIZWITS_LOG("OnBoarding: SoftAP or Web mode\n");
            }

            if(1 == status->ststus.types.station)
            {
                gizwitsProtocol.wifiStatusEvent.event[gizwitsProtocol.wifiStatusEvent.num] = WIFI_AIRLINK;
                gizwitsProtocol.wifiStatusEvent.num++;
                GIZWITS_LOG("OnBoarding: AirLink mode\n");
            }
        }
        else
        {
            if(1 == status->ststus.types.softap)
            {
                gizwitsProtocol.wifiStatusEvent.event[gizwitsProtocol.wifiStatusEvent.num] = WIFI_SOFTAP;
                gizwitsProtocol.wifiStatusEvent.num++;
                GIZWITS_LOG("OnBoarding: SoftAP or Web mode\n");
            }

            if(1 == status->ststus.types.station)
            {
                gizwitsProtocol.wifiStatusEvent.event[gizwitsProtocol.wifiStatusEvent.num] = WIFI_STATION;
                gizwitsProtocol.wifiStatusEvent.num++;
                GIZWITS_LOG("OnBoarding: Station mode\n");
            }
        }
    }

    //binding mode status
    if(lastStatus.types.binding != status->ststus.types.binding)
    {
        lastStatus.types.binding = status->ststus.types.binding;
        if(1 == status->ststus.types.binding)
        {
            gizwitsProtocol.wifiStatusEvent.event[gizwitsProtocol.wifiStatusEvent.num] = WIFI_OPEN_BINDING;
            gizwitsProtocol.wifiStatusEvent.num++;
            GIZWITS_LOG("WiFi status: in binding mode\n");
        }
        else
        {
            gizwitsProtocol.wifiStatusEvent.event[gizwitsProtocol.wifiStatusEvent.num] = WIFI_CLOSE_BINDING;
            gizwitsProtocol.wifiStatusEvent.num++;
            GIZWITS_LOG("WiFi status: out binding mode\n");
        }
    }

    //router status
    if(lastStatus.types.con_route != status->ststus.types.con_route)
    {
        lastStatus.types.con_route = status->ststus.types.con_route;
        if(1 == status->ststus.types.con_route)
        {
            gizwitsProtocol.wifiStatusEvent.event[gizwitsProtocol.wifiStatusEvent.num] = WIFI_CON_ROUTER;
            gizwitsProtocol.wifiStatusEvent.num++;
            GIZWITS_LOG("WiFi status: connected router\n");
        }
        else
        {
            gizwitsProtocol.wifiStatusEvent.event[gizwitsProtocol.wifiStatusEvent.num] = WIFI_DISCON_ROUTER;
            gizwitsProtocol.wifiStatusEvent.num++;
            GIZWITS_LOG("WiFi status: disconnected router\n");
        }
    }

    //M2M server status
    if(lastStatus.types.con_m2m != status->ststus.types.con_m2m)
    {
        lastStatus.types.con_m2m = status->ststus.types.con_m2m;
        if(1 == status->ststus.types.con_m2m)
        {
            gizwitsProtocol.wifiStatusEvent.event[gizwitsProtocol.wifiStatusEvent.num] = WIFI_CON_M2M;
            gizwitsProtocol.wifiStatusEvent.num++;
            GIZWITS_LOG("WiFi status: connected m2m\n");
        }
        else
        {
            gizwitsProtocol.wifiStatusEvent.event[gizwitsProtocol.wifiStatusEvent.num] = WIFI_DISCON_M2M;
            gizwitsProtocol.wifiStatusEvent.num++;
            GIZWITS_LOG("WiFi status: disconnected m2m\n");
        }
    }

    //APP status
    if(lastStatus.types.app != status->ststus.types.app)
    {
        lastStatus.types.app = status->ststus.types.app;
        if(1 == status->ststus.types.app)
        {
            gizwitsProtocol.wifiStatusEvent.event[gizwitsProtocol.wifiStatusEvent.num] = WIFI_CON_APP;
            gizwitsProtocol.wifiStatusEvent.num++;
            GIZWITS_LOG("WiFi status: app connect\n");
        }
        else
        {
            gizwitsProtocol.wifiStatusEvent.event[gizwitsProtocol.wifiStatusEvent.num] = WIFI_DISCON_APP;
            gizwitsProtocol.wifiStatusEvent.num++;
            GIZWITS_LOG("WiFi status: no app connect\n");
        }
    }

    //test mode status
    if(lastStatus.types.test != status->ststus.types.test)
    {
        lastStatus.types.test = status->ststus.types.test;
        if(1 == status->ststus.types.test)
        {
            gizwitsProtocol.wifiStatusEvent.event[gizwitsProtocol.wifiStatusEvent.num] = WIFI_OPEN_TESTMODE;
            gizwitsProtocol.wifiStatusEvent.num++;
            GIZWITS_LOG("WiFi status: in test mode\n");
        }
        else
        {
            gizwitsProtocol.wifiStatusEvent.event[gizwitsProtocol.wifiStatusEvent.num] = WIFI_CLOSE_TESTMODE;
            gizwitsProtocol.wifiStatusEvent.num++;
            GIZWITS_LOG("WiFi status: out test mode\n");
        }
    }

    gizwitsProtocol.wifiStatusEvent.event[gizwitsProtocol.wifiStatusEvent.num] = WIFI_RSSI;
    gizwitsProtocol.wifiStatusEvent.num++;
    gizwitsProtocol.wifiStatusData.rssi = status->ststus.types.rssi;
    GIZWITS_LOG("RSSI is %d \n", gizwitsProtocol.wifiStatusData.rssi);

    gizwitsProtocol.issuedFlag = WIFI_STATUS_TYPE;

    return 0;
}


/**@name Gizwits User API interface
* @{
*/

/**
* @brief gizwits Protocol initialization interface

* Protocol-related timer, serial port initialization

* Datapoint initialization

* @param none
* @return none
*/
void gizwitsInit(void)
{    
    pRb.rbCapacity = RB_MAX_LEN;
    pRb.rbBuff = rbBuf;
    if(0 == rbCreate(&pRb))
	{
		GIZWITS_LOG("rbCreate Success \n");
	}
	else
	{
		GIZWITS_LOG("rbCreate Faild \n");
	}
    
    memset((uint8_t *)&gizwitsProtocol, 0, sizeof(gizwitsProtocol_t));
}

/**
* @brief WiFi configure interface

* Set the WiFi module into the corresponding configuration mode or reset the module

* @param[in] mode ：0x0， reset the module ;0x01， SoftAp mode ;0x02， AirLink mode ;0x03， Production test mode; 0x04:allow users to bind devices

* @return Error command code
*/
int32_t gizwitsSetMode(uint8_t mode)
{
    int32_t ret = 0;
    protocolCfgMode_t cfgMode;
    protocolCommon_t setDefault;

    switch(mode)
    {
        case WIFI_RESET_MODE:
            gizProtocolHeadInit((protocolHead_t *)&setDefault);
            setDefault.head.cmd = CMD_SET_DEFAULT;
            setDefault.head.sn = gizwitsProtocol.sn++;
            setDefault.head.len = exchangeBytes(sizeof(protocolCommon_t)-4);
            setDefault.sum = gizProtocolSum((uint8_t *)&setDefault, sizeof(protocolCommon_t));
            ret = uartWrite((uint8_t *)&setDefault, sizeof(protocolCommon_t));
            if(ret < 0)
            {
                GIZWITS_LOG("ERR: uart write error %d \n", ret);
            }

            gizProtocolWaitAck((uint8_t *)&setDefault, sizeof(protocolCommon_t));   
            break;
        case WIFI_SOFTAP_MODE:
            gizProtocolHeadInit((protocolHead_t *)&cfgMode);
            cfgMode.head.cmd = CMD_WIFI_CONFIG;
            cfgMode.head.sn = gizwitsProtocol.sn++;
            cfgMode.cfgMode = mode;
            cfgMode.head.len = exchangeBytes(sizeof(protocolCfgMode_t)-4);
            cfgMode.sum = gizProtocolSum((uint8_t *)&cfgMode, sizeof(protocolCfgMode_t));
            ret = uartWrite((uint8_t *)&cfgMode, sizeof(protocolCfgMode_t));
            if(ret < 0)
            {
                GIZWITS_LOG("ERR: uart write error %d \n", ret);
            }
            gizProtocolWaitAck((uint8_t *)&cfgMode, sizeof(protocolCfgMode_t)); 
            break;
        case WIFI_AIRLINK_MODE:
            gizProtocolHeadInit((protocolHead_t *)&cfgMode);
            cfgMode.head.cmd = CMD_WIFI_CONFIG;
            cfgMode.head.sn = gizwitsProtocol.sn++;
            cfgMode.cfgMode = mode;
            cfgMode.head.len = exchangeBytes(sizeof(protocolCfgMode_t)-4);
            cfgMode.sum = gizProtocolSum((uint8_t *)&cfgMode, sizeof(protocolCfgMode_t));
            ret = uartWrite((uint8_t *)&cfgMode, sizeof(protocolCfgMode_t));
            if(ret < 0)
            {
                GIZWITS_LOG("ERR: uart write error %d \n", ret);
            }
            gizProtocolWaitAck((uint8_t *)&cfgMode, sizeof(protocolCfgMode_t)); 
            break;
        case WIFI_PRODUCTION_TEST:
            gizProtocolHeadInit((protocolHead_t *)&setDefault);
            setDefault.head.cmd = CMD_PRODUCTION_TEST;
            setDefault.head.sn = gizwitsProtocol.sn++;
            setDefault.head.len = exchangeBytes(sizeof(protocolCommon_t)-4);
            setDefault.sum = gizProtocolSum((uint8_t *)&setDefault, sizeof(protocolCommon_t));
            ret = uartWrite((uint8_t *)&setDefault, sizeof(protocolCommon_t));
            if(ret < 0)
            {
                GIZWITS_LOG("ERR: uart write error %d \n", ret);
            }

            gizProtocolWaitAck((uint8_t *)&setDefault, sizeof(protocolCommon_t));
            break;
        case WIFI_NINABLE_MODE:
            gizProtocolHeadInit((protocolHead_t *)&setDefault);
            setDefault.head.cmd = CMD_NINABLE_MODE;
            setDefault.head.sn = gizwitsProtocol.sn++;
            setDefault.head.len = exchangeBytes(sizeof(protocolCommon_t)-4);
            setDefault.sum = gizProtocolSum((uint8_t *)&setDefault, sizeof(protocolCommon_t));
            ret = uartWrite((uint8_t *)&setDefault, sizeof(protocolCommon_t));
            if(ret < 0)
            {
                GIZWITS_LOG("ERR: uart write error %d \n", ret);
            }

            gizProtocolWaitAck((uint8_t *)&setDefault, sizeof(protocolCommon_t)); 
            break;
        case WIFI_REBOOT_MODE:
            gizProtocolHeadInit((protocolHead_t *)&setDefault);
            setDefault.head.cmd = CMD_REBOOT_MODULE;
            setDefault.head.sn = gizwitsProtocol.sn++;
            setDefault.head.len = exchangeBytes(sizeof(protocolCommon_t)-4);
            setDefault.sum = gizProtocolSum((uint8_t *)&setDefault, sizeof(protocolCommon_t));
            ret = uartWrite((uint8_t *)&setDefault, sizeof(protocolCommon_t));
            if(ret < 0)
            {
                GIZWITS_LOG("ERR: uart write error %d \n", ret);
            }

            gizProtocolWaitAck((uint8_t *)&setDefault, sizeof(protocolCommon_t)); 
            break;
        default:
            GIZWITS_LOG("ERR: CfgMode error!\n");
            break;
    }

    return ret;
}

/**
* @brief Get the the network time

* Protocol 4.13:"Device MCU send" of "the MCU requests access to the network time"

* @param[in] none
* @return none
*/
void gizwitsGetNTP(void)
{
    int32_t ret = 0;
    protocolCommon_t getNTP;

    gizProtocolHeadInit((protocolHead_t *)&getNTP);
    getNTP.head.cmd = CMD_GET_NTP;
    getNTP.head.sn = gizwitsProtocol.sn++;
    getNTP.head.len = exchangeBytes(sizeof(protocolCommon_t)-4);
    getNTP.sum = gizProtocolSum((uint8_t *)&getNTP, sizeof(protocolCommon_t));
    ret = uartWrite((uint8_t *)&getNTP, sizeof(protocolCommon_t));
    if(ret < 0)
    {
        GIZWITS_LOG("ERR[NTP]: uart write error %d \n", ret);
    }

    gizProtocolWaitAck((uint8_t *)&getNTP, sizeof(protocolCommon_t));
}


/**
* @brief Get Module Info

* 

* @param[in] none
* @return none
*/
void gizwitsGetModuleInfo(void)
{
    int32_t ret = 0;
    protocolGetModuleInfo_t getModuleInfo;

    gizProtocolHeadInit((protocolHead_t *)&getModuleInfo);
    getModuleInfo.head.cmd = CMD_ASK_MODULE_INFO;
    getModuleInfo.head.sn = gizwitsProtocol.sn++;
    getModuleInfo.type = 0x0;
    getModuleInfo.head.len = exchangeBytes(sizeof(protocolGetModuleInfo_t)-4);
    getModuleInfo.sum = gizProtocolSum((uint8_t *)&getModuleInfo, sizeof(protocolGetModuleInfo_t));
    ret = uartWrite((uint8_t *)&getModuleInfo, sizeof(protocolGetModuleInfo_t));
    if(ret < 0)
    {
        GIZWITS_LOG("ERR[NTP]: uart write error %d \n", ret);
    }

    gizProtocolWaitAck((uint8_t *)&getModuleInfo, sizeof(protocolGetModuleInfo_t));
}


/**
* @brief Module Info Analyse
*
* @param [in] head : 
*
* @return 0, Success， , other,Faild
*/
static int8_t gizProtocolModuleInfoHandle(protocolHead_t *head)
{
    protocolModuleInfo_t *moduleInfo = (protocolModuleInfo_t *)head;

    if(NULL == head)
    {
        GIZWITS_LOG("NTP is empty \n");
        return -1;
    }
#if MODULE_TYPE
    memcpy((uint8_t *)&gizwitsProtocol.gprsInfoNews,(uint8_t *)&moduleInfo->gprsModuleInfo, sizeof(gprsInfo_t));
#else
    memcpy((uint8_t *)&gizwitsProtocol.wifiModuleNews,(uint8_t *)&moduleInfo->wifiModuleInfo, sizeof(moduleInfo_t));
#endif
    gizwitsProtocol.moduleInfoEvent.event[gizwitsProtocol.moduleInfoEvent.num] = MODULE_INFO;
    gizwitsProtocol.moduleInfoEvent.num++;

    gizwitsProtocol.issuedFlag = GET_MODULEINFO_TYPE;


    return 0;
}

/**
* @brief Protocol handling function

* 

* @param [in] currentData :The protocol data pointer
* @return none
*/
int32_t gizwitsHandle(dataPoint_t *currentData)
{
    int8_t ret = 0;
#ifdef PROTOCOL_DEBUG
    uint16_t i = 0;
#endif
    uint8_t ackData[RB_MAX_LEN];
    uint16_t protocolLen = 0;
    uint32_t ackLen = 0;
    protocolHead_t *recvHead = NULL;
    char *didPtr = NULL;
    uint16_t offset = 0;
#ifdef OTA_ENABLE
    int8_t updatePieceResult = 0;
#endif    

    if(NULL == currentData)
    {
        GIZWITS_LOG("GizwitsHandle Error , Illegal Param\n");
        return -1;
    }

    /*resend strategy*/
    gizProtocolAckHandle();
    ret = gizProtocolGetOnePacket(&pRb, gizwitsProtocol.protocolBuf, &protocolLen);

    if(0 == ret)
    {
        GIZWITS_LOG("Get One Packet!\n");
        
#ifdef PROTOCOL_DEBUG
        GIZWITS_LOG("WiFi2MCU[%4d:%4d]: ", gizGetTimerCount(), protocolLen);
        for(i=0; i<protocolLen;i++)
        {
            GIZWITS_LOG("%02x ", gizwitsProtocol.protocolBuf[i]);
        }
        GIZWITS_LOG("\n");
#endif

        recvHead = (protocolHead_t *)gizwitsProtocol.protocolBuf;
        switch (recvHead->cmd)
        {
            case CMD_GET_DEVICE_INTO:
                gizProtocolGetDeviceInfo(recvHead);
                break;
            case CMD_ISSUED_P0:
                GIZWITS_LOG("flag %x %x \n", recvHead->flags[0], recvHead->flags[1]);
                //offset = 1;
               
                if(0 == gizProtocolIssuedProcess(didPtr, gizwitsProtocol.protocolBuf+sizeof(protocolHead_t)+offset, protocolLen-(sizeof(protocolHead_t)+offset+1), ackData, &ackLen))
                {
                    gizProtocolIssuedDataAck(recvHead, ackData, ackLen,recvHead->flags[1]);
                    GIZWITS_LOG("AckData : \n");
                }
                break;
            case CMD_HEARTBEAT:
                gizProtocolCommonAck(recvHead);
                break;
            case CMD_WIFISTATUS:
                gizProtocolCommonAck(recvHead);
                gizProtocolModuleStatus((protocolWifiStatus_t *)recvHead);
                break;
            case ACK_REPORT_P0:
            case ACK_WIFI_CONFIG:
            case ACK_SET_DEFAULT:
            case ACK_NINABLE_MODE:
            case ACK_REBOOT_MODULE:
                gizProtocolWaitAckCheck(recvHead);
                break;
            case CMD_MCU_REBOOT:
                gizProtocolCommonAck(recvHead);
                GIZWITS_LOG("report:MCU reboot!\n");
                
                gizProtocolReboot();
                break;
            case CMD_ERROR_PACKAGE:
                break;
            case ACK_PRODUCTION_TEST:
                gizProtocolWaitAckCheck(recvHead);
                GIZWITS_LOG("Ack PRODUCTION_MODE success \n");
                break;           
            case ACK_GET_NTP:
                gizProtocolWaitAckCheck(recvHead);
                gizProtocolNTP(recvHead);
                GIZWITS_LOG("Ack GET_UTT success \n");
                break; 
            case ACK_ASK_MODULE_INFO:
                gizProtocolWaitAckCheck(recvHead);
                gizProtocolModuleInfoHandle(recvHead);
                GIZWITS_LOG("Ack GET_Module success \n");
                break;
#ifdef OTA_ENABLE
            case ACK_BIGDATA_READY:
                gizProtocolWaitAckCheck(recvHead);
                break;
            case ACK_D_STOP_BIGDATA_SEND:
                gizProtocolWaitAckCheck(recvHead);
                break;
            case CMD_ASK_BIGDATA:
                GIZWITS_LOG("CMD_ASK_BIGDATA \n");
                gizProtocolCommonAck(recvHead);
                if(0 == Pro_W2D_UpdateCmdHandle((uint8_t *)gizwitsProtocol.protocolBuf + sizeof(protocolHead_t), protocolLen - sizeof(protocolCommon_t)))
                {
                    romUpdate.otaBusyModeFlag = 1;
                    GIZWITS_LOG("System In OTA Mode BusyFlag = %d \n", romUpdate.otaBusyModeFlag);
                    //GIZWITS_LOG("CMD_ASK_BIGDATA\n");
                }
                else
                {
                    GIZWITS_LOG("Update Ask Handle Failed \n");
                    Pro_D2W_UpdateSuspend();
                }

                break;
            case CMD_BIGDATA_SEND:
                gizProtocolCommonAck(recvHead);
                updatePieceResult = Pro_W2D_UpdateDataHandle((uint8_t *)gizwitsProtocol.protocolBuf + sizeof(protocolHead_t), protocolLen - sizeof(protocolCommon_t),HEX);
                if(0 != updatePieceResult)
                {
                    romUpdate.otaBusyModeFlag = 0;
                    GIZWITS_LOG("CMD_BIGDATA_SEND , Piece Handle Faild . System OTA Mode Over BusyFlag = %d \n",romUpdate.otaBusyModeFlag);
                    Pro_D2W_UpdateSuspend();
                    GIZWITS_LOG("System Go On \n");
                }
                break;
            case CMD_S_STOP_BIGDATA_SEND:
                gizProtocolCommonAck(recvHead);
                romUpdate.otaBusyModeFlag = 0;
                GIZWITS_LOG("System OTA Mode Over BusyFlag = %d \n",romUpdate.otaBusyModeFlag);
            break;
#endif 
            default:
                gizProtocolErrorCmd(recvHead,ERROR_CMD);
                GIZWITS_LOG("ERR: cmd code error!\n");
                break;
        }
    }
    else if(-2 == ret)
    {
        //Check failed, report exception
        recvHead = (protocolHead_t *)gizwitsProtocol.protocolBuf;
        gizProtocolErrorCmd(recvHead,ERROR_ACK_SUM);
        GIZWITS_LOG("ERR: check sum error!\n");
        return -2;
    }
    
    switch(gizwitsProtocol.issuedFlag)
    {
        case ACTION_CONTROL_TYPE:
            gizwitsProtocol.issuedFlag = STATELESS_TYPE;
            gizwitsEventProcess(&gizwitsProtocol.issuedProcessEvent, (uint8_t *)&gizwitsProtocol.gizCurrentDataPoint, sizeof(dataPoint_t));
            memset((uint8_t *)&gizwitsProtocol.issuedProcessEvent,0x0,sizeof(gizwitsProtocol.issuedProcessEvent));  
            break;
        case WIFI_STATUS_TYPE:
            gizwitsProtocol.issuedFlag = STATELESS_TYPE;
            gizwitsEventProcess(&gizwitsProtocol.wifiStatusEvent, (uint8_t *)&gizwitsProtocol.wifiStatusData, sizeof(moduleStatusInfo_t));
            memset((uint8_t *)&gizwitsProtocol.wifiStatusEvent,0x0,sizeof(gizwitsProtocol.wifiStatusEvent));
            break;
        case ACTION_W2D_TRANSPARENT_TYPE:
            gizwitsProtocol.issuedFlag = STATELESS_TYPE;
            gizwitsEventProcess(&gizwitsProtocol.issuedProcessEvent, (uint8_t *)gizwitsProtocol.transparentBuff, gizwitsProtocol.transparentLen);
            break;
        case GET_NTP_TYPE:
            gizwitsProtocol.issuedFlag = STATELESS_TYPE;
            gizwitsEventProcess(&gizwitsProtocol.NTPEvent, (uint8_t *)&gizwitsProtocol.TimeNTP, sizeof(protocolTime_t));
            memset((uint8_t *)&gizwitsProtocol.NTPEvent,0x0,sizeof(gizwitsProtocol.NTPEvent));
            break;
        case GET_MODULEINFO_TYPE:
            gizwitsProtocol.issuedFlag = STATELESS_TYPE;
            gizwitsEventProcess(&gizwitsProtocol.moduleInfoEvent, (uint8_t *)&gizwitsProtocol.wifiModuleNews, sizeof(moduleInfo_t));
            memset((uint8_t *)&gizwitsProtocol.moduleInfoEvent,0x0,sizeof(moduleInfo_t));
            break;
        default:
            break;      
    }
#ifdef OTA_ENABLE
    if (romUpdate.otaBusyModeFlag == 0)
    {
        gizDevReportPolicy(currentData);
    }
#else
 
    gizDevReportPolicy(currentData);
#endif
 
    return 0;
}

/**
* @brief gizwits report transparent data interface

* The user can call the interface to complete the reporting of private protocol data

* @param [in] data :Private protocol data
* @param [in] len  :Private protocol data length
* @return 0，success ;other，failure
*/
int32_t gizwitsPassthroughData(uint8_t * gizdata, uint32_t len)
{
	int32_t ret = 0;
	uint8_t tx_buf[MAX_PACKAGE_LEN];
	uint8_t *pTxBuf = tx_buf;
	uint16_t data_len = 6+len;
    if(NULL == gizdata)
    {
        GIZWITS_LOG("[ERR] gizwitsPassthroughData Error \n");
        return (-1);
    }

	*pTxBuf ++= 0xFF;
	*pTxBuf ++= 0xFF;
	*pTxBuf ++= (uint8_t)(data_len>>8);//len
	*pTxBuf ++= (uint8_t)(data_len);
	*pTxBuf ++= CMD_REPORT_P0;//0x1b cmd
	*pTxBuf ++= gizwitsProtocol.sn++;//sn
	*pTxBuf ++= 0x00;//flag
	*pTxBuf ++= 0x00;//flag
	*pTxBuf ++= ACTION_D2W_TRANSPARENT_DATA;//P0_Cmd

    memcpy(&tx_buf[9],gizdata,len);
    tx_buf[data_len + 4 - 1 ] = gizProtocolSum( tx_buf , (data_len+4));
    
	ret = uartWrite(tx_buf, data_len+4);
    if(ret < 0)
    {
        GIZWITS_LOG("ERR: uart write error %d \n", ret);
    }

    gizProtocolWaitAck(tx_buf, data_len+4);

    return 0;
}

#ifdef OTA_ENABLE
/**
* @brief Pro_W2D_UpdateCmdHandle
* Handle OTA Ask , Transform MD5 Char2Hex
* @param[in]  :
* @param[out] :
* @return  0,Update Ask Handle Success , Send Ready Success
*		  -1,Input Param Illegal
*		  -2,Update Ask Handle Success , Send Ready Faild
*
*/
int8_t Pro_W2D_UpdateCmdHandle(uint8_t *inData,uint32_t dataLen)
{
    uint8_t k = 0;
    int8_t ret = 0;

    uint8_t  fileMD5value[FILE_MD5_MAX_LEN];
    uint16_t fileMD5len;			//MD5 Length

    if(NULL == inData)
    {
        return -1;
    }

    romUpdate.updateFileSize = ((uint32_t)(inData[0]<<24))|((uint32_t)(inData[1]<<16))|((uint32_t)(inData[2]<<8))|((uint32_t)(inData[3]));

    //judge flash size
    if(romUpdate.updateFileSize > APP_BAK_DATA_MAX_SIZE)
    {
        GIZWITS_LOG("UpdateFileSize Error ,Update Failed ,fileSize = %d \n",romUpdate.updateFileSize);
        return -1;
    }
    else
    {
        GIZWITS_LOG("UpdateFileSize Legal ,size = %d \n",romUpdate.updateFileSize);
        romUpdate.update_param.rom_size = romUpdate.updateFileSize;
        flash_erase(APP_BAK_DATA_MAX_SIZE,SYS_APP_BAK_SAVE_ADDR_BASE);
		GIZWITS_LOG("flash erase finished!!\n");
    }

    fileMD5len = inData[4]*256 + inData[5];
    GIZWITS_LOG("FileMD5len  = %d ", fileMD5len);

    memcpy(fileMD5value,&inData[6],fileMD5len);
#ifdef PROTOCOL_DEBUG
    GIZWITS_LOG("MD5: ");
    for(uint16_t i=0; i<32; i++)
    {
        GIZWITS_LOG("%02x ", fileMD5value[i]);
    }
    GIZWITS_LOG("\r\n");
#endif

    for(uint16_t j = 0; j<SSL_MAX_LEN; j++)
    {
        romUpdate.update_param.ssl_data[j] = char2hex(fileMD5value[k],fileMD5value[k+1]);
        k += 2;
    }
#ifdef PROTOCOL_DEBUG
    GIZWITS_LOG("MD5_Hex: ");
    for(uint16_t i=0; i<SSL_MAX_LEN; i++)
    {
        GIZWITS_LOG("%02x ", romUpdate.update_param.ssl_data[i]);
    }
    GIZWITS_LOG("\r\n");
#endif

    GIZWITS_LOG("GAgent_MD5Init \n");
    GAgent_MD5Init(&romUpdate.ctx);

    //send ready
    ret = Pro_D2W_UpdateReady(fileMD5value,fileMD5len);
    if(0 != ret)
    {
        GIZWITS_LOG("Pro_D2W_UpdateReady Error ,Error Code = %d \n",ret);
        return -2;
    }

    return 0;
}

/**
* @brief Pro_D2W_UpdateReady
* MCU Send Update Ready
* @param[in]  md5Data: Input md5 char data
* @param[in]  md5Len : Input md5 length
* @param[out] :
* @return  0,Update Ask Handle Success , Send Ready Success
*		  -1,Input Param Illegal
*		  -2,Uart Send Faild
*
*/
int8_t Pro_D2W_UpdateReady(uint8_t *md5Data , uint16_t md5Len)
{
    int8_t ret = 0;
    uint8_t txBuf[100];
		memset(txBuf,0,100);
    uint8_t *pTxBuf = txBuf;
    if(NULL == md5Data)
    {
        return -1;
    }

    uint16_t dataLen = sizeof(protocolCommon_t) + 2 + md5Len + 2 - 4 ;
    *pTxBuf ++= 0xFF;
    *pTxBuf ++= 0xFF;
    *pTxBuf ++= (uint8_t)(dataLen>>8);
    *pTxBuf ++= (uint8_t)(dataLen);
    *pTxBuf ++= CMD_BIGDATA_READY;
    *pTxBuf ++= gizwitsProtocol.sn++;

    *pTxBuf ++= 0x00;//flag
    txBuf[7] |= UPDATE_IS_HEX_FORMAT<<0;//TERRY WARNING
    pTxBuf += 1;

    *pTxBuf ++= (uint8_t)(md5Len>>8);//len
    *pTxBuf ++= (uint8_t)(md5Len);

    memcpy(&txBuf[8 + 2],md5Data,md5Len);
    pTxBuf += md5Len;

    *pTxBuf ++= (uint8_t)(PIECE_MAX_LEN>>8);//len
    *pTxBuf ++= (uint8_t)(PIECE_MAX_LEN);
    *pTxBuf ++= gizProtocolSum(txBuf , (dataLen+4));

    ret = uartWrite(txBuf , (dataLen+4));
    if(ret < 0)
    {
        GIZWITS_LOG("ERROR: uart write error %d \n", ret);
        return -2;
    }
    GIZWITS_LOG("MCU Ready To Update ROM \n");
    return 0;
}

/**
* @brief Pro_W2D_UpdateDataHandle
* update Piece Handle ,  Judge Last Piece
* @param[in] indata   : Piece Data
* @param[in] dataLen    : Piece Length
* @param[in] formatType : Piece Data Format
* @param[out]
* @return  0,Handle Success
*		  -1,Input Param Illegal
*		  -2,Last Piece , MD5 Check Faild
*
*/
int8_t Pro_W2D_UpdateDataHandle(uint8_t *inData , uint32_t dataLen , otaDataType formatType)
{
    uint16_t piecenum = 0;
    uint16_t piececount = 0;
    uint32_t tempWFlashAddr = 0;
    updataPieceData_TypeDef pieceData;
    uint8_t md5_calc[SSL_MAX_LEN];//MD5 Calculate Fact

    if(NULL == inData)
    {
        return -1;
    }

    memcpy((uint8_t *)&pieceData, inData, dataLen);

    piecenum = exchangeBytes(pieceData.piecenum);
    piececount = exchangeBytes(pieceData.piececount);

    GIZWITS_LOG("******piecenum = %d , piececount = %d, pieceSize = %d******** \r\n",piecenum,piececount,dataLen - 4);


    tempWFlashAddr = SYS_APP_BAK_SAVE_ADDR_BASE + (piecenum-1) * PIECE_MAX_LEN;
    wFlashData((uint8_t *)pieceData.piececontent , dataLen - 4, tempWFlashAddr);

    GAgent_MD5Update(&romUpdate.ctx, (uint8_t *)pieceData.piececontent, dataLen - 4);

    /*updata package data ,ack*/
    if(piecenum == piececount)
    {
        memset(md5_calc,0,SSL_MAX_LEN);
        GAgent_MD5Final(&romUpdate.ctx, md5_calc);
        GIZWITS_LOG("MD5 Calculate Success , Will Check The MD5 ..\n ");

        if(0 != memcmp(romUpdate.update_param.ssl_data, md5_calc, SSL_MAX_LEN))
        {
            GIZWITS_LOG("Md5_Cacl Check Faild ,MCU OTA Faild\r\n ");
#ifdef PROTOCOL_DEBUG
            GIZWITS_LOG("Calc MD5: ");
            for(uint16_t i=0; i<SSL_MAX_LEN; i++)
            {
                GIZWITS_LOG("%02x ", md5_calc[i]);
            }
            GIZWITS_LOG("\r\n");
#endif
#ifdef PROTOCOL_DEBUG
            GIZWITS_LOG("SSL MD5: ");
            for(uint16_t i=0; i<SSL_MAX_LEN; i++)
            {
                GIZWITS_LOG("%02x ", romUpdate.update_param.ssl_data[i]);
            }
            GIZWITS_LOG("\r\n");
#endif
            memset((uint8_t *)&romUpdate.update_param,0,sizeof(updateParamSave_t));

            return -2;
        }
        else
        {
            GIZWITS_LOG("MD5 Check Success ,Storage  ROM Success , Write Update Flag\n ");
            flash_erase(sizeof(updateParamSave_t) , UPDATE_PARAM_SAVE_ADDR_BASE);

            romUpdate.update_param.rom_statue = 0xEEEE;
            wFlashData((uint8_t *)&romUpdate.update_param, sizeof(updateParamSave_t), UPDATE_PARAM_SAVE_ADDR_BASE);
						//romUpdate.update_param.rom_statue = 0x1234;
						//printf("\n\romUpdate.update_param.rom_statue = %04X \r\n\n",romUpdate.update_param.rom_statue);
						//memset((uint8_t *)&romUpdate, 0 , sizeof(romUpdate));
						//rFlashData((uint8_t *)&romUpdate, sizeof(romUpdate), UPDATE_PARAM_SAVE_ADDR_BASE);
						//printf("\n\romUpdate.update_param.rom_statue = %04X \r\n\n",romUpdate.update_param.rom_statue);
					
            GIZWITS_LOG("System Will Restart... \n");
            /****************************MCU RESTART****************************/
            mcuRestart();
            /******************************************************************************/
            //last package , updata ok
            //MD5 checkout :Failed clear updata,Success , write flash ,begin updata
        }
    }
    return 0;
}

/**
* @brief Pro_D2W_UpdateSuspend
* Data Receiver
* @param[in]    : Void
* @param[out] 	:
* @return  		: Void
*/
void Pro_D2W_UpdateSuspend()
{
    int32_t ret = 0;
    protocolCommon_t protocolCommon;
    memset(&protocolCommon, 0, sizeof(protocolCommon_t));
    gizProtocolHeadInit((protocolHead_t *)&protocolCommon);
    protocolCommon.head.len = exchangeBytes(sizeof(protocolCommon_t)-4);
    protocolCommon.head.cmd = CMD_D_STOP_BIGDATA_SEND;
    protocolCommon.head.sn = gizwitsProtocol.sn++;
    protocolCommon.sum = gizProtocolSum((uint8_t *)&protocolCommon, sizeof(protocolCommon_t));

    ret = uartWrite((uint8_t *)&protocolCommon,sizeof(protocolCommon_t));
    if(ret < 0)
    {
        GIZWITS_LOG("ERROR: uart write error %d \n", ret);
    }

    gizProtocolWaitAck((uint8_t *)&protocolCommon,sizeof(protocolCommon_t));

}

/**
* @brief Pro_D2W_Ask_Module_Reboot
* Ask Module Reboot
* @param[in]    : Void
* @param[out] 	:
* @return  		: Void
*/
void Pro_D2W_Ask_Module_Reboot()
{
    int32_t ret = 0;
    protocolCommon_t protocolCommon;
    memset(&protocolCommon, 0, sizeof(protocolCommon_t));
    gizProtocolHeadInit((protocolHead_t *)&protocolCommon);
    protocolCommon.head.len = exchangeBytes(sizeof(protocolCommon_t)-4);
    protocolCommon.head.cmd = CMD_REBOOT_MODULE;
    protocolCommon.head.sn = gizwitsProtocol.sn++;
    protocolCommon.sum = gizProtocolSum((uint8_t *)&protocolCommon, sizeof(protocolCommon_t));

    ret = uartWrite((uint8_t *)&protocolCommon,sizeof(protocolCommon_t));
    if(ret < 0)
    {
        GIZWITS_LOG("ERROR: uart write error %d \n", ret);
    }

    gizProtocolWaitAck((uint8_t *)&protocolCommon,sizeof(protocolCommon_t));

}
#endif

/**@} */
