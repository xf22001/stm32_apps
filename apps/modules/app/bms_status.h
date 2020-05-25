

/*================================================================
 *   
 *   
 *   文件名称：bms_status.h
 *   创 建 者：肖飞
 *   创建日期：2020年05月25日 星期一 15时31分42秒
 *   修改日期：2020年05月25日 星期一 15时32分23秒
 *   描    述：
 *
 *================================================================*/
#ifndef _BMS_STATUS_H
#define _BMS_STATUS_H
#ifdef __cplusplus
extern "C"
{
#endif

#ifdef __cplusplus
}
#endif

typedef enum {
	RETURN_ERROR = 0,         //无
	BRM_TIMEOUT,              //BRM报文超时
	BCP_TIMEOUT,              //BCP超时
	BRO_TIMEOUT,              //BRO超时
	BCS_TIMEOUT,              //BCS超时
	BCL_TIMEOUT,              //BCL超时
	BST_TIMEOUT,              //BST超时
	BSD_TIMEOUT,              //BSD超时
	BRO_ERROR,                //BRO未准备就绪
	BMS_SUCCESS,              //BMS通讯正常

	BMS_UNCONNECT,            //BMS未连接
	RETURN_INC_TIMEOUT,       //绝缘检测超时
	RETURN_INC_ERROR,         //绝缘故障
	RETURN_DISCHARGE_TIMEOUT, //泄放电路超时
	RETURN_DISCHARGE_ERROR,   //泄放失败
	RETURN_COCON_TIMEOUT,     //粘连检测超时
	RETURN_COCON_ERROR,       //粘连检测失败
	BCS_SUCCESS,              //接收BCS成功
	RETURN_INC_BMS_ERROR,     //检测绝缘时前段带电
	BMS_STARTING,             //正在启动

	CRO_TIMEOUT,              //20检测车上电压超时
	PRECHARGE_ERROR,          //预充错误
	BMS_U_UNMATCH,            //车辆电压不匹配
	SHORT_CIRCUIT_ERROR,      //短路故障
	BFC_ERR,                  //CFC数据错误
	BRO_ABNORMAL,             //BRO异常

	RETURN_SUCCESS = 0xff,
} bms_status_t;


#endif //_BMS_STATUS_H
