

/*================================================================
 *
 *
 *   文件名称：request_ocpp_1_6.c
 *   创 建 者：肖飞
 *   创建日期：2021年07月08日 星期四 14时19分21秒
 *   修改日期：2021年07月13日 星期二 17时05分58秒
 *   描    述：
 *
 *================================================================*/
#include "net_client.h"

#include "command_status.h"
#include "channels.h"

#include "websocket.h"

#include "log.h"

typedef enum {
	//[2, "19223201", "BootNotification", {"chargePointVendor": "VendorX", "chargePointModel": "SingleSocketCharger"}]
	OCPP_MSG_TYPE_CALL = 2,//[<MessageTypeId>, "<UniqueId>", "<Action>", {<Payload>}]
	//[3, "19223201", {"status":"Accepted", "currentTime":"2013-02-01T20:53:32.486Z", "heartbeatInterval":300}]
	OCPP_MSG_TYPE_CALL_RESULT,//[<MessageTypeId>, "<UniqueId>", {<Payload>}]
	OCPP_MSG_TYPE_CALL_ERROR,//[<MessageTypeId>, "<UniqueId>", "<errorCode>", "<errorDescription>", {<errorDetails>}]
} ocpp_msg_type_t;

#define CHARGING_SCHEDULE_MAX_PERIODS 6

#define ocpp_msg_type_call_error_code(code) OCPP_MSG_TYPE_CALL_ERROR_CODE_##code

typedef enum {
	ocpp_msg_type_call_error_code(NotImplemented) = 0,//Requested Action is not known by receiver 13Error Code Description
	ocpp_msg_type_call_error_code(NotSupported),//Requested Action is recognized but not supported by the receiver
	ocpp_msg_type_call_error_code(InternalError),//An internal error occurred and the receiver was not able to process the requested Action
	ocpp_msg_type_call_error_code(successfully),
	ocpp_msg_type_call_error_code(ProtocolError),//Payload for Action is incomplete
	ocpp_msg_type_call_error_code(SecurityError),//During the processing of Action a security issue occurred preventing receiver from completing the Action successfully
	ocpp_msg_type_call_error_code(FormationViolation),//Payload for Action is syntactically incorrect or not conform the PDU structure for Action
	ocpp_msg_type_call_error_code(PropertyConstraintViolation),//Payload is syntactically correct but at least one field contains an invalid value
	ocpp_msg_type_call_error_code(OccurenceConstraintViolation),//Payload for Action is syntactically correct but at least one of the fields violates occurence constraints
	ocpp_msg_type_call_error_code(TypeConstraintViolation),//Payload for Action is syntactically correct but at least one of the fields violates data type constraints (e.g. “somestring”: 12)
	ocpp_msg_type_call_error_code(GenericError),//Any other error not covered by the previous ones
} ocpp_msg_type_call_error_code_t;

#define add_ocpp_msg_type_call_error_code_case(code) \
		case ocpp_msg_type_call_error_code(code): { \
			des = #code; \
		} \
		break

static char *get_ocpp_msg_type_call_error_code_des(ocpp_msg_type_call_error_code_t code)
{
	char *des = "unknow";

	switch(code) {
			add_ocpp_msg_type_call_error_code_case(NotImplemented);//Requested Action is not known by receiver 13Error Code Description
			add_ocpp_msg_type_call_error_code_case(NotSupported);//Requested Action is recognized but not supported by the receiver
			add_ocpp_msg_type_call_error_code_case(InternalError);//An internal error occurred and the receiver was not able to process the requested Action
			add_ocpp_msg_type_call_error_code_case(successfully);
			add_ocpp_msg_type_call_error_code_case(ProtocolError);//Payload for Action is incomplete
			add_ocpp_msg_type_call_error_code_case(SecurityError);//During the processing of Action a security issue occurred preventing receiver from completing the Action successfully
			add_ocpp_msg_type_call_error_code_case(FormationViolation);//Payload for Action is syntactically incorrect or not conform the PDU structure for Action
			add_ocpp_msg_type_call_error_code_case(PropertyConstraintViolation);//Payload is syntactically correct but at least one field contains an invalid value
			add_ocpp_msg_type_call_error_code_case(OccurenceConstraintViolation);//Payload for Action is syntactically correct but at least one of the fields violates occurence constraints
			add_ocpp_msg_type_call_error_code_case(TypeConstraintViolation);//Payload for Action is syntactically correct but at least one of the fields violates data type constraints (e.g. “somestring”: 12)
			add_ocpp_msg_type_call_error_code_case(GenericError);//Any other error not covered by the previous ones

		default: {
		}
		break;
	}

	return des;
}

#define enum_ocpp_command(command) ENUM_OCPP_COMMAND_##command

typedef enum {
	//设备-->服务器
	enum_ocpp_command(Authorize) = 0,//鉴权(刷卡)
	enum_ocpp_command(BootNotification),//登录
	enum_ocpp_command(Heartbeat),//心跳包
	enum_ocpp_command(MeterValues),//电表参数
	enum_ocpp_command(StartTransaction),//开始充电信息
	enum_ocpp_command(StatusNotification),//状态推送
	enum_ocpp_command(StopTransaction),//停止充电信息
	enum_ocpp_command(FirmwareStatusNotification),//升级状态推送
	enum_ocpp_command(DiagnosticsStatusNotification),//诊断信息状态推送

	//服务器-->设备
	enum_ocpp_command(ChangeAvailability),//充电桩使能开关
	enum_ocpp_command(ChangeConfiguration),//改变充电桩本地配置
	enum_ocpp_command(ClearCache),//清除证书缓存
	enum_ocpp_command(GetConfiguration),//获取充电桩本地配置
	enum_ocpp_command(RemoteStartTransaction),//远程启动充电
	enum_ocpp_command(RemoteStopTransaction),//远程停止充电
	enum_ocpp_command(Reset),//重启充电桩
	enum_ocpp_command(UnlockConnector),//充电桩解除锁定
	enum_ocpp_command(UpdateFirmware),//升级固件
	enum_ocpp_command(GetDiagnostics),//获取诊断信息
	enum_ocpp_command(GetLocalListVersion),//获取本地列表版本号
	enum_ocpp_command(SendLocalList),//下发本地列表
	enum_ocpp_command(CancelReservation),//取消预约
	enum_ocpp_command(ReserveNow),//现在预约
	enum_ocpp_command(ClearChargingProfile),//清除充电配置文件
	enum_ocpp_command(GetCompositeSchedule),//获取综合时间表
	enum_ocpp_command(SetChargingProfile),//设置充电配置文件
	enum_ocpp_command(TriggerMessage),//远程触发上报指定消息

	//远程触发上报指定消息
	enum_ocpp_command(DataTransfer),//数据传输(OCPP中不包含的数据)
	ENUM_OCPP_COMMAND_SIZE,//数据传输(OCPP中不包含的数据)
} ocpp_command_t;

#define add_ocpp_command_name_case(command) \
		case enum_ocpp_command(command): { \
			name = #command; \
		} \
		break

static char *get_ocpp_command_name(ocpp_command_t command)
{
	char *name = "unknow";

	switch(command) {
			//设备-->服务器
			add_ocpp_command_name_case(Authorize);//鉴权(刷卡)
			add_ocpp_command_name_case(BootNotification);//登录
			add_ocpp_command_name_case(Heartbeat);//心跳包
			add_ocpp_command_name_case(MeterValues);//电表参数
			add_ocpp_command_name_case(StartTransaction);//开始充电信息
			add_ocpp_command_name_case(StatusNotification);//状态推送
			add_ocpp_command_name_case(StopTransaction);//停止充电信息
			add_ocpp_command_name_case(FirmwareStatusNotification);//升级状态推送
			add_ocpp_command_name_case(DiagnosticsStatusNotification);//诊断信息状态推送

			//服务器-->设备
			add_ocpp_command_name_case(ChangeAvailability);//充电桩使能开关
			add_ocpp_command_name_case(ChangeConfiguration);//改变充电桩本地配置
			add_ocpp_command_name_case(ClearCache);//清除证书缓存
			add_ocpp_command_name_case(GetConfiguration);//获取充电桩本地配置
			add_ocpp_command_name_case(RemoteStartTransaction);//远程启动充电
			add_ocpp_command_name_case(RemoteStopTransaction);//远程停止充电
			add_ocpp_command_name_case(Reset);//重启充电桩
			add_ocpp_command_name_case(UnlockConnector);//充电桩解除锁定
			add_ocpp_command_name_case(UpdateFirmware);//升级固件
			add_ocpp_command_name_case(GetDiagnostics);//获取诊断信息
			add_ocpp_command_name_case(GetLocalListVersion);//获取本地列表版本号
			add_ocpp_command_name_case(SendLocalList);//下发本地列表
			add_ocpp_command_name_case(CancelReservation);//取消预约
			add_ocpp_command_name_case(ReserveNow);//现在预约
			add_ocpp_command_name_case(ClearChargingProfile);//清除充电配置文件
			add_ocpp_command_name_case(GetCompositeSchedule);//获取综合时间表
			add_ocpp_command_name_case(SetChargingProfile);//设置充电配置文件
			add_ocpp_command_name_case(TriggerMessage);//远程触发上报指定消息

			//远程触发上报指定消息
			add_ocpp_command_name_case(DataTransfer);//数据传输(OCPP中不包含的数据)

		default: {
		}
		break;
	}

	return name;
}

#define enum_ocpp_measurand1(measurand1) ENUM_OCPP_MEASURAND_##measurand1
#define enum_ocpp_measurand2(measurand1, measurand2) ENUM_OCPP_MEASURAND_##measurand1##measurand2
#define enum_ocpp_measurand3(measurand1, measurand2, measurand3) ENUM_OCPP_MEASURAND_##measurand1##measurand2##measurand3
#define enum_ocpp_measurand4(measurand1, measurand2, measurand3, measurand4) ENUM_OCPP_MEASURAND_##measurand1##measurand2##measurand3##measurand4

typedef enum {
	enum_ocpp_measurand2(Current, Export) = 0,//Instantaneous current flow from EV,车端流出的瞬间电流
	enum_ocpp_measurand2(Current, Import),//Instantaneous current flow to EV,流向车端的瞬间电流
	enum_ocpp_measurand2(Current, Offered),//Maximum current offered to EV(提供给电动汽车的最大电流
	enum_ocpp_measurand4(Energy, Active, Export, Register),//Energy exported by EV (Wh or kWh),车端输出的电能
	enum_ocpp_measurand4(Energy, Active, Import, Register),//Energy imported by EV (Wh or kWh),车端输入的电能
	enum_ocpp_measurand4(Energy, Reactive, Export, Register),//Reactive energy exported by EV (varh or kvarh),电动汽车输出的无功电能
	enum_ocpp_measurand4(Energy, Reactive, Import, Register),//Reactive energy imported by EV (varh or kvarh),电动汽车输入的无功电能
	enum_ocpp_measurand4(Energy, Active, Export, Interval),//Energy exported by EV (Wh or kWh),车端输出的电能
	enum_ocpp_measurand4(Energy, Active, Import, Interval),//Energy imported by EV (Wh or kWh),车端输入的电能
	enum_ocpp_measurand4(Energy, Reactive, Export, Interval),//Reactive energy exported by EV (varh or kvarh),电动汽车输出的无功电能
	enum_ocpp_measurand4(Energy, Reactive, Import, Interval),//Reactive energy imported by EV (varh or kvarh),电动汽车输入的无功电能
	enum_ocpp_measurand1(Frequency),//Instantaneous reading of powerline frequency,电力线频率的瞬时读数
	enum_ocpp_measurand3(Power, Active, Export),//Instantaneous active power exported by EV. (W or kW)电动汽车输出的瞬时有功功率。
	enum_ocpp_measurand3(Power, Active, Import),//Instantaneous active power Imported by EV. (W or kW)电动汽车输入的瞬时有功功率。
	enum_ocpp_measurand2(Power, Factor),//Instantaneous power factor of total energy flow,总能量流的瞬时功率因数
	enum_ocpp_measurand2(Power, Offered),//Maximum power offered to EV,Maximum power offered to EV
	enum_ocpp_measurand3(Power, Reactive, Export),//Instantaneous reactive power exported by EV. (var or kvar),电动汽车输出的瞬时无功功率。
	enum_ocpp_measurand3(Power, Reactive, Import),//Instantaneous reactive power Imported by EV. (var or kvar),电动汽车输入的瞬时无功功率。
	enum_ocpp_measurand1(RPM),//Fan speed in RPM, 风扇转速
	enum_ocpp_measurand1(SoC),//State of charge of charging vehicle in percentage,充电车辆的充电状态,百分比.
	enum_ocpp_measurand1(Temperature),//Temperature reading inside Charge Point,充电点内部的温度读数。
	enum_ocpp_measurand1(Voltage),//Instantaneous AC RMS supply voltage, 瞬时AC RMS电源电压
	enum_ocpp_measurand1(StartS),//State of charge of charging vehicle in percentage,充电车辆的充电状态,百分比.
	enum_ocpp_measurand1(EndS),//State of charge of charging vehicle in percentage,充电车辆的充电状态,百分比.
} ocpp_measurand_t;

#define add_ocpp_measurand_name_case1(measurand1) \
		case enum_ocpp_measurand1(measurand1): { \
			name = #measurand1; \
		} \
		break
#define add_ocpp_measurand_name_case2(measurand1, measurand2) \
		case enum_ocpp_measurand2(measurand1, measurand2): { \
			name = #measurand1"."#measurand2; \
		} \
		break
#define add_ocpp_measurand_name_case3(measurand1, measurand2, measurand3) \
		case enum_ocpp_measurand3(measurand1, measurand2, measurand3): { \
			name = #measurand1"."#measurand2"."#measurand3; \
		} \
		break
#define add_ocpp_measurand_name_case4(measurand1, measurand2, measurand3, measurand4) \
		case enum_ocpp_measurand4(measurand1, measurand2, measurand3, measurand4): { \
			name = #measurand1"."#measurand2"."#measurand3"."#measurand4; \
		} \
		break

static char *get_ocpp_measurand_name(ocpp_measurand_t measurand)
{
	char *name = "unknow";

	switch(measurand) {
			add_ocpp_measurand_name_case2(Current, Export);//Instantaneous current flow from EV,车端流出的瞬间电流
			add_ocpp_measurand_name_case2(Current, Import);//Instantaneous current flow to EV,流向车端的瞬间电流
			add_ocpp_measurand_name_case2(Current, Offered);//Maximum current offered to EV(提供给电动汽车的最大电流
			add_ocpp_measurand_name_case4(Energy, Active, Export, Register);//Energy exported by EV (Wh or kWh);车端输出的电能
			add_ocpp_measurand_name_case4(Energy, Active, Import, Register);//Energy imported by EV (Wh or kWh);车端输入的电能
			add_ocpp_measurand_name_case4(Energy, Reactive, Export, Register);//Reactive energy exported by EV (varh or kvarh);电动汽车输出的无功电能
			add_ocpp_measurand_name_case4(Energy, Reactive, Import, Register);//Reactive energy imported by EV (varh or kvarh);电动汽车输入的无功电能
			add_ocpp_measurand_name_case4(Energy, Active, Export, Interval);//Energy exported by EV (Wh or kWh);车端输出的电能
			add_ocpp_measurand_name_case4(Energy, Active, Import, Interval);//Energy imported by EV (Wh or kWh);车端输入的电能
			add_ocpp_measurand_name_case4(Energy, Reactive, Export, Interval);//Reactive energy exported by EV (varh or kvarh);电动汽车输出的无功电能
			add_ocpp_measurand_name_case4(Energy, Reactive, Import, Interval);//Reactive energy imported by EV (varh or kvarh);电动汽车输入的无功电能
			add_ocpp_measurand_name_case1(Frequency);//Instantaneous reading of powerline frequency,电力线频率的瞬时读数
			add_ocpp_measurand_name_case3(Power, Active, Export);//Instantaneous active power exported by EV. (W or kW)电动汽车输出的瞬时有功功率。
			add_ocpp_measurand_name_case3(Power, Active, Import);//Instantaneous active power Imported by EV. (W or kW)电动汽车输入的瞬时有功功率。
			add_ocpp_measurand_name_case2(Power, Factor);//Instantaneous power factor of total energy flow,总能量流的瞬时功率因数
			add_ocpp_measurand_name_case2(Power, Offered);//Maximum power offered to EV,Maximum power offered to EV
			add_ocpp_measurand_name_case3(Power, Reactive, Export);//Instantaneous reactive power exported by EV. (var or kvar);电动汽车输出的瞬时无功功率。
			add_ocpp_measurand_name_case3(Power, Reactive, Import);//Instantaneous reactive power Imported by EV. (var or kvar);电动汽车输入的瞬时无功功率。
			add_ocpp_measurand_name_case1(RPM);//Fan speed in RPM, 风扇转速
			add_ocpp_measurand_name_case1(SoC);//State of charge of charging vehicle in percentage,充电车辆的充电状态,百分比.
			add_ocpp_measurand_name_case1(Temperature);//Temperature reading inside Charge Point,充电点内部的温度读数。
			add_ocpp_measurand_name_case1(Voltage);//Instantaneous AC RMS supply voltage, 瞬时AC RMS电源电压
			add_ocpp_measurand_name_case1(StartS);//State of charge of charging vehicle in percentage,充电车辆的充电状态,百分比.
			add_ocpp_measurand_name_case1(EndS);//State of charge of charging vehicle in percentage,充电车辆的充电状态,百分比.

		default: {
		}
		break;
	}

	return name;
}

#define enum_ocpp_charger_status(status) ENUM_OCPP_CHARGER_STATUS_##status

typedef enum {
	enum_ocpp_charger_status(None) = 0,
	enum_ocpp_charger_status(Available),
	enum_ocpp_charger_status(Preparing),
	enum_ocpp_charger_status(Charging),
	enum_ocpp_charger_status(SuspendedEVSE),
	enum_ocpp_charger_status(SuspendedEV),
	enum_ocpp_charger_status(Finishing),
	enum_ocpp_charger_status(Reserved),
	enum_ocpp_charger_status(Unavailable),
	enum_ocpp_charger_status(Faulted),
} ocpp_charger_status_t;

#define add_ocpp_charger_status_case(status) \
		case enum_ocpp_charger_status(status): { \
			des = #status; \
		} \
		break

static char *get_ocpp_charger_status_des(ocpp_charger_status_t status)
{
	char *des = "unknow";

	switch(status) {
			add_ocpp_charger_status_case(None);
			add_ocpp_charger_status_case(Available);
			add_ocpp_charger_status_case(Preparing);
			add_ocpp_charger_status_case(Charging);
			add_ocpp_charger_status_case(SuspendedEVSE);
			add_ocpp_charger_status_case(SuspendedEV);
			add_ocpp_charger_status_case(Finishing);
			add_ocpp_charger_status_case(Reserved);
			add_ocpp_charger_status_case(Unavailable);
			add_ocpp_charger_status_case(Faulted);

		default: {
		}
		break;
	}

	return des;
}

#define enum_ocpp_error_code(code) ENUM_OCPP_ERROR_CODE_##code

typedef enum {
	enum_ocpp_error_code(ConnectorLockFailure) = 0,//开/关锁失败
	enum_ocpp_error_code(EVCommunicationError),//与车辆通信失败
	enum_ocpp_error_code(GroundFailure),//接地故障
	enum_ocpp_error_code(HighTemperature),//桩过热
	enum_ocpp_error_code(InternalError),//内部软硬件错误
	enum_ocpp_error_code(LocalListConflict),//云端授权与本地列表冲突
	enum_ocpp_error_code(NoError),
	enum_ocpp_error_code(OtherError),
	enum_ocpp_error_code(OverCurrentFailure),//过电流保护装置已跳闸。
	enum_ocpp_error_code(OverVoltage),//电压已升至可接受水平以上。
	enum_ocpp_error_code(PowerMeterFailure),//无法读取功率计。
	enum_ocpp_error_code(PowerSwitchFailure),//无法控制电源开关。
	enum_ocpp_error_code(ReaderFailure),//idTag阅读器失败。
	enum_ocpp_error_code(ResetFailure),//重启失败
	enum_ocpp_error_code(UnderVoltage),//电压过低
	enum_ocpp_error_code(WeakSignal),//无线通信设备报告信号较弱。
} ocpp_error_code_t;

#define add_ocpp_error_code_case(code) \
		case enum_ocpp_error_code(code): { \
			des = #code; \
		} \
		break

static char *get_ocpp_error_code_des(ocpp_error_code_t code)
{
	char *des = "unknow";

	switch(code) {
			add_ocpp_error_code_case(ConnectorLockFailure);//开/关锁失败
			add_ocpp_error_code_case(EVCommunicationError);//与车辆通信失败
			add_ocpp_error_code_case(GroundFailure);//接地故障
			add_ocpp_error_code_case(HighTemperature);//桩过热
			add_ocpp_error_code_case(InternalError);//内部软硬件错误
			add_ocpp_error_code_case(LocalListConflict);//云端授权与本地列表冲突
			add_ocpp_error_code_case(NoError);
			add_ocpp_error_code_case(OtherError);
			add_ocpp_error_code_case(OverCurrentFailure);//过电流保护装置已跳闸。
			add_ocpp_error_code_case(OverVoltage);//电压已升至可接受水平以上。
			add_ocpp_error_code_case(PowerMeterFailure);//无法读取功率计。
			add_ocpp_error_code_case(PowerSwitchFailure);//无法控制电源开关。
			add_ocpp_error_code_case(ReaderFailure);//idTag阅读器失败。
			add_ocpp_error_code_case(ResetFailure);//重启失败
			add_ocpp_error_code_case(UnderVoltage);//电压过低
			add_ocpp_error_code_case(WeakSignal);//无线通信设备报告信号较弱。

		default: {
		}
		break;
	}

	return des;
}

#define enum_ocpp_stop_reason(reason) ENUM_OCPP_STOP_REASON_##reason

typedef enum {
	enum_ocpp_stop_reason(EmergencyStop) = 0,
	enum_ocpp_stop_reason(EVDisconnected),
	enum_ocpp_stop_reason(HardReset),
	enum_ocpp_stop_reason(Local),
	enum_ocpp_stop_reason(Other),
	enum_ocpp_stop_reason(PowerLoss),
	enum_ocpp_stop_reason(Reboot),
	enum_ocpp_stop_reason(Remote),
	enum_ocpp_stop_reason(SoftReset),
	enum_ocpp_stop_reason(UnlockCommand),
	enum_ocpp_stop_reason(DeAuthorized),
} ocpp_stop_reason_t;

#define add_ocpp_stop_reason_case(reason) \
		case enum_ocpp_stop_reason(reason): { \
			des = #reason; \
		} \
		break

static char *get_ocpp_stop_reason_des(ocpp_stop_reason_t reason)
{
	char *des = "unknow";

	switch(reason) {
			add_ocpp_stop_reason_case(EmergencyStop);
			add_ocpp_stop_reason_case(EVDisconnected);
			add_ocpp_stop_reason_case(HardReset);
			add_ocpp_stop_reason_case(Local);
			add_ocpp_stop_reason_case(Other);
			add_ocpp_stop_reason_case(PowerLoss);
			add_ocpp_stop_reason_case(Reboot);
			add_ocpp_stop_reason_case(Remote);
			add_ocpp_stop_reason_case(SoftReset);
			add_ocpp_stop_reason_case(UnlockCommand);
			add_ocpp_stop_reason_case(DeAuthorized);

		default: {
		}
		break;
	}

	return des;
}

#define enum_ocpp_vendor_error_code(code) OCPP_VENDOR_ERROR_CODE_##code

typedef enum {
	enum_ocpp_vendor_error_code(OK) = 0, //系统正常
	enum_ocpp_vendor_error_code(POWER_ERR),//模块异常
	enum_ocpp_vendor_error_code(CARD_BOARD_ERR),//刷卡板异常
	enum_ocpp_vendor_error_code(CHANNEL_B_ERR),//控制板B异常
	enum_ocpp_vendor_error_code(CHANNEL_C_ERR),//控制板C异常
	enum_ocpp_vendor_error_code(CHANNEL_D_ERR),//控制板D异常
	enum_ocpp_vendor_error_code(FUN_A_ERR),//功能板A异常
	enum_ocpp_vendor_error_code(FUN_B_ERR),//功能板B异常
	enum_ocpp_vendor_error_code(FUN_C_ERR),//功能板C异常
	enum_ocpp_vendor_error_code(FUN_D_ERR),//功能板D异常
	enum_ocpp_vendor_error_code(ELCMETER_A_ERR),//电表A异常10
	enum_ocpp_vendor_error_code(ELCMETER_B_ERR),//电表B异常
	enum_ocpp_vendor_error_code(ELCMETER_C_ERR),//电表C异常
	enum_ocpp_vendor_error_code(ELCMETER_D_ERR),//电表D异常
	enum_ocpp_vendor_error_code(Abnormal_Access_Control),//门禁异常
	enum_ocpp_vendor_error_code(CONTACTOR_ADHESION),//接触器粘连15
	enum_ocpp_vendor_error_code(Emergency_Stop),//急停
	enum_ocpp_vendor_error_code(INPUT_OVERVOLTAGE),//输入过压
	enum_ocpp_vendor_error_code(INPUT_UNDERVOLTAGE),//输入欠压
	enum_ocpp_vendor_error_code(GUN_HIGH_TEMPERATURE),//枪头温度高
} ocpp_vendor_error_code_t;

#define add_ocpp_vendor_error_code_case(code) \
		case enum_ocpp_vendor_error_code(code): { \
			des = #code; \
		} \
		break

static char *get_ocpp_vendor_error_code_des(ocpp_vendor_error_code_t code)
{
	char *des = "unknow";

	switch(code) {
			add_ocpp_vendor_error_code_case(OK); //系统正常
			add_ocpp_vendor_error_code_case(POWER_ERR);//模块异常
			add_ocpp_vendor_error_code_case(CARD_BOARD_ERR);//刷卡板异常
			add_ocpp_vendor_error_code_case(CHANNEL_B_ERR);//控制板B异常
			add_ocpp_vendor_error_code_case(CHANNEL_C_ERR);//控制板C异常
			add_ocpp_vendor_error_code_case(CHANNEL_D_ERR);//控制板D异常
			add_ocpp_vendor_error_code_case(FUN_A_ERR);//功能板A异常
			add_ocpp_vendor_error_code_case(FUN_B_ERR);//功能板B异常
			add_ocpp_vendor_error_code_case(FUN_C_ERR);//功能板C异常
			add_ocpp_vendor_error_code_case(FUN_D_ERR);//功能板D异常
			add_ocpp_vendor_error_code_case(ELCMETER_A_ERR);//电表A异常10
			add_ocpp_vendor_error_code_case(ELCMETER_B_ERR);//电表B异常
			add_ocpp_vendor_error_code_case(ELCMETER_C_ERR);//电表C异常
			add_ocpp_vendor_error_code_case(ELCMETER_D_ERR);//电表D异常
			add_ocpp_vendor_error_code_case(Abnormal_Access_Control);//门禁异常
			add_ocpp_vendor_error_code_case(CONTACTOR_ADHESION);//接触器粘连15
			add_ocpp_vendor_error_code_case(Emergency_Stop);//急停
			add_ocpp_vendor_error_code_case(INPUT_OVERVOLTAGE);//输入过压
			add_ocpp_vendor_error_code_case(INPUT_UNDERVOLTAGE);//输入欠压
			add_ocpp_vendor_error_code_case(GUN_HIGH_TEMPERATURE);//枪头温度高

		default: {
		}
		break;
	}

	return des;
}

#define enum_ocpp_response_status(status) OCPP_RESPONSE_STATUS_##status

typedef enum {
	enum_ocpp_response_status(Accepted) = 0,
	enum_ocpp_response_status(Rejected),
	enum_ocpp_response_status(Failed),
	enum_ocpp_response_status(Scheduled),
	enum_ocpp_response_status(RebootRequired),
	enum_ocpp_response_status(NotSupported),
	enum_ocpp_response_status(UnknownMessageId),
	enum_ocpp_response_status(UnknownVendorId),
	enum_ocpp_response_status(VersionMismatch),
	enum_ocpp_response_status(Faulted),
	enum_ocpp_response_status(Occupied),
	enum_ocpp_response_status(Unavailable),
	enum_ocpp_response_status(Unknown),
	enum_ocpp_response_status(NotImplemented),
} ocpp_response_status_t;

#define add_ocpp_response_status_case(status) \
		case enum_ocpp_response_status(status): { \
			des = #status; \
		} \
		break

static char *get_ocpp_response_status_des(ocpp_response_status_t status)
{
	char *des = "unknow";

	switch(status) {
			add_ocpp_response_status_case(Accepted);
			add_ocpp_response_status_case(Rejected);
			add_ocpp_response_status_case(Failed);
			add_ocpp_response_status_case(Scheduled);
			add_ocpp_response_status_case(RebootRequired);
			add_ocpp_response_status_case(NotSupported);
			add_ocpp_response_status_case(UnknownMessageId);
			add_ocpp_response_status_case(UnknownVendorId);
			add_ocpp_response_status_case(VersionMismatch);
			add_ocpp_response_status_case(Faulted);
			add_ocpp_response_status_case(Occupied);
			add_ocpp_response_status_case(Unavailable);
			add_ocpp_response_status_case(Unknown);
			add_ocpp_response_status_case(NotImplemented);

		default: {
		}
		break;
	}

	return des;
}

#define enum_ocpp_core_key(key) OCPP_CORE_KEY_##key

typedef enum {
	/* required */
	/* Core */
	enum_ocpp_core_key(AuthorizeRemoteTxRequests) = 0,//read false
	enum_ocpp_core_key(ClockAlignedDataInterval),//read/write int 1800(30min)
	enum_ocpp_core_key(ConnectionTimeOut),//read/write int 60(1min)
	enum_ocpp_core_key(GetConfigurationMaxKeys),///read int 16
	enum_ocpp_core_key(HeartbeatInterval),//read/write int 60(1min)
	enum_ocpp_core_key(LocalAuthorizeOffline),//read/write bool false
	enum_ocpp_core_key(LocalPreAuthorize),//read/write bool false
	enum_ocpp_core_key(MeterValuesAlignedData),//read/write string ""
	enum_ocpp_core_key(MeterValuesSampledData),//read/write string ""
	enum_ocpp_core_key(MeterValueSampleInterval),//read/write int 20
	enum_ocpp_core_key(NumberOfConnectors),//read int
	enum_ocpp_core_key(ResetRetries),//read/write int 3
	enum_ocpp_core_key(ConnectorPhaseRotation),//read/write string ""
	enum_ocpp_core_key(StopTransactionOnEVSideDisconnect),//read/write bool false
	enum_ocpp_core_key(StopTransactionOnInvalidId),//read/write bool true
	enum_ocpp_core_key(StopTxnAlignedData),//read/write string ""
	enum_ocpp_core_key(StopTxnSampledData),//read/write string ""
	enum_ocpp_core_key(SupportedFeatureProfiles),//read string "core"
	enum_ocpp_core_key(TransactionMessageAttempts),//read/write int 10
	enum_ocpp_core_key(TransactionMessageRetryInterval),//read/write int 3
	enum_ocpp_core_key(UnlockConnectorOnEVSideDisconnect),//read/write bool false

	/* Local Auth List Management Profile */
	enum_ocpp_core_key(LocalAuthListEnabled),//read/write bool false
	enum_ocpp_core_key(LocalAuthListMaxLength),//read/write int 20
	enum_ocpp_core_key(SendLocalListMaxLength),//read/write int 20

	/* Smart Charging Profile */
	enum_ocpp_core_key(ChargeProfileMaxStackLevel),//read int 0
	enum_ocpp_core_key(ChargingScheduleAllowedChargingRateUnit),//read string ""
	enum_ocpp_core_key(ChargingScheduleMaxPeriods),//read int CHARGING_SCHEDULE_MAX_PERIODS
	enum_ocpp_core_key(MaxChargingProfilesInstalled),//read int 1
	/* Optional */
	enum_ocpp_core_key(WebSocketPingInterval),//read/write int 30

	enum_ocpp_core_key(SIZE),
} ocpp_core_key_t;

#define add_ocpp_core_key_case(key) \
		case enum_ocpp_core_key(key): { \
			name = #key; \
		} \
		break

static char *get_ocpp_core_key_name(ocpp_core_key_t key)
{
	char *name = "unknow";

	switch(key) {
			/* required */
			/* Core */
			add_ocpp_core_key_case(AuthorizeRemoteTxRequests);//read false
			add_ocpp_core_key_case(ClockAlignedDataInterval);//read/write int 1800(30min)
			add_ocpp_core_key_case(ConnectionTimeOut);//read/write int 60(1min)
			add_ocpp_core_key_case(GetConfigurationMaxKeys);///read int 16
			add_ocpp_core_key_case(HeartbeatInterval);//read/write int 60(1min)
			add_ocpp_core_key_case(LocalAuthorizeOffline);//read/write bool false
			add_ocpp_core_key_case(LocalPreAuthorize);//read/write bool false
			add_ocpp_core_key_case(MeterValuesAlignedData);//read/write string ""
			add_ocpp_core_key_case(MeterValuesSampledData);//read/write string ""
			add_ocpp_core_key_case(MeterValueSampleInterval);//read/write int 20
			add_ocpp_core_key_case(NumberOfConnectors);//read int
			add_ocpp_core_key_case(ResetRetries);//read/write int 3
			add_ocpp_core_key_case(ConnectorPhaseRotation);//read/write string ""
			add_ocpp_core_key_case(StopTransactionOnEVSideDisconnect);//read/write bool false
			add_ocpp_core_key_case(StopTransactionOnInvalidId);//read/write bool true
			add_ocpp_core_key_case(StopTxnAlignedData);//read/write string ""
			add_ocpp_core_key_case(StopTxnSampledData);//read/write string ""
			add_ocpp_core_key_case(SupportedFeatureProfiles);//read string "core"
			add_ocpp_core_key_case(TransactionMessageAttempts);//read/write int 10
			add_ocpp_core_key_case(TransactionMessageRetryInterval);//read/write int 3
			add_ocpp_core_key_case(UnlockConnectorOnEVSideDisconnect);//read/write bool false

			/* Local Auth List Management Profile */
			add_ocpp_core_key_case(LocalAuthListEnabled);//read/write bool false
			add_ocpp_core_key_case(LocalAuthListMaxLength);//read/write int 20
			add_ocpp_core_key_case(SendLocalListMaxLength);//read/write int 20

			/* Smart Charging Profile */
			add_ocpp_core_key_case(ChargeProfileMaxStackLevel);//read int 0
			add_ocpp_core_key_case(ChargingScheduleAllowedChargingRateUnit);//read string ""
			add_ocpp_core_key_case(ChargingScheduleMaxPeriods);//read int 6
			add_ocpp_core_key_case(MaxChargingProfilesInstalled);//read int 1
			/* Optional */
			add_ocpp_core_key_case(WebSocketPingInterval);//read/write int 30

		default: {
		}
		break;
	}

	return name;
}

#define enum_occp_firmware_update_status(status) OCCP_FIRMWARE_UPDATE_STATUS_##status

typedef enum {
	enum_occp_firmware_update_status(None) = 0,
	enum_occp_firmware_update_status(Downloaded),
	enum_occp_firmware_update_status(DownloadFailed),
	enum_occp_firmware_update_status(Downloading),
	enum_occp_firmware_update_status(Idle),
	enum_occp_firmware_update_status(InstallationFailed),
	enum_occp_firmware_update_status(Installing),
	enum_occp_firmware_update_status(Installed),
} occp_firmware_update_status_t;

#define add_occp_firmware_update_status_case(status) \
		case enum_occp_firmware_update_status(status): { \
			des = #status; \
		} \
		break

static char *get_occp_firmware_update_status_des(occp_firmware_update_status_t status)
{
	char *des = "unknow";

	switch(status) {
			add_occp_firmware_update_status_case(None);
			add_occp_firmware_update_status_case(Downloaded);
			add_occp_firmware_update_status_case(DownloadFailed);
			add_occp_firmware_update_status_case(Downloading);
			add_occp_firmware_update_status_case(Idle);
			add_occp_firmware_update_status_case(InstallationFailed);
			add_occp_firmware_update_status_case(Installing);
			add_occp_firmware_update_status_case(Installed);

		default: {
		}
		break;
	}

	return des;
}

#define enum_occp_diagnostics_upload_status(status) OCCP_DIAGNOSTICS_UPLOAD_STATUS_##status

typedef enum {
	enum_occp_diagnostics_upload_status(None) = 0,
	enum_occp_diagnostics_upload_status(Idle),
	enum_occp_diagnostics_upload_status(Uploaded),
	enum_occp_diagnostics_upload_status(UploadFailed),
	enum_occp_diagnostics_upload_status(Uploading),
} occp_diagnostics_upload_status_t;

#define add_occp_diagnostics_upload_status_case(status) \
		case enum_occp_diagnostics_upload_status(status): { \
			des = #status; \
		} \
		break

static char *get_occp_diagnostics_upload_status_des(occp_diagnostics_upload_status_t status)
{
	char *des = "unknow";

	switch(status) {
			add_occp_diagnostics_upload_status_case(None);
			add_occp_diagnostics_upload_status_case(Idle);
			add_occp_diagnostics_upload_status_case(Uploaded);
			add_occp_diagnostics_upload_status_case(UploadFailed);
			add_occp_diagnostics_upload_status_case(Uploading);

		default: {
		}
		break;
	}

	return des;
}

//Smart Charging
typedef struct {
	int start_period;
	int limit;//限制值。原参数带小数，收到时乘10以消除1位小数, 单位为0.1w/a。
	int number_phases;
} ocpp_schedule_period_t;

typedef struct {
	uint8_t period_id;
	int duration;
	int start_schedule;
	uint8_t charging_rate_unit;
	int min_charging_rate;//最小限制值。原参数带小数，收到时乘10以消除1位小数。
	ocpp_schedule_period_t ocpp_schedule_period[CHARGING_SCHEDULE_MAX_PERIODS];
} ocpp_charging_schedule_t;

typedef struct {
	uint8_t id;
	uint8_t valid;
	uint8_t connector_id;
	uint32_t transaction_id;
	uint8_t stack_level;
	uint8_t charging_profile_purpose;
	uint8_t charging_profile_kind;
	uint8_t recurrency_kind;
	time_t start;
	time_t end;
	ocpp_charging_schedule_t ocpp_charging_schedule;
} ocpp_charging_profile_t;

typedef struct {
	command_status_t *channel_cmd_ctx;
} net_client_channel_data_ctx_t;

typedef struct {
	channels_info_t *channels_info;
	uint8_t request_timeout;
	uint8_t ocpp_key[enum_ocpp_core_key(SIZE)];

	command_status_t *device_cmd_ctx;
	net_client_channel_data_ctx_t *channel_data_ctx;
} net_client_data_ctx_t;

typedef enum {
	NET_CLIENT_DEVICE_COMMAND_NONE = 0,
} net_client_device_command_t;

typedef enum {
	NET_CLIENT_CHANNEL_COMMAND_NONE = 0,
} net_client_channel_command_t;

typedef int (*net_client_request_callback_t)(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id, uint8_t *send_buffer, uint16_t send_buffer_size);
typedef int (*net_client_response_callback_t)(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id, uint8_t *request, uint16_t request_size, uint8_t *send_buffer, uint16_t send_buffer_size);
typedef int (*net_client_timeout_callback_t)(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id);

typedef struct {
	uint8_t cmd;
	uint32_t periodic;
	uint16_t frame;
	net_client_request_callback_t request_callback;
	net_client_response_callback_t response_callback;
	net_client_timeout_callback_t timeout_callback;
} net_client_command_item_t;

static net_client_data_ctx_t *net_client_data_ctx = NULL;

static int send_frame(net_client_info_t *net_client_info, uint8_t *data, size_t size, uint8_t *send_buffer, size_t send_buffer_size)
{
	int ret = -1;
	int sent = 0;
	int retry = 0;

	ret = ws_encode(data, size, (uint8_t *)send_buffer, &send_buffer_size, 1, WS_OPCODE_TXT, 1);

	if(ret != 0) {
		debug("");
		return ret;
	}

	//_hexdump("send_frame", (const char *)send_buffer, send_buffer_size);

	while(sent < send_buffer_size) {
		ret = send_to_server(net_client_info, (uint8_t *)send_buffer + sent, send_buffer_size - sent);

		if(ret > 0) {
			retry = 0;
			sent += ret;
			ret = sent;
		} else if(ret == 0) {
			retry++;

			if(retry >= 128) {
				debug("");
				ret = -1;
				break;
			}
		} else {
			debug("ret:%d", ret);
			set_client_state(net_client_info, CLIENT_RESET);
			break;
		}
	}

	return ret;
}

static net_client_command_item_t *net_client_command_item_device_table[] = {
};

static net_client_command_item_t *net_client_command_item_channel_table[] = {
};

static void ocpp_1_6_ctrl_cmd(void *_net_client_info, void *_ctrl_cmd_info)
{
	//net_client_info_t *net_client_info = (net_client_info_t *)_net_client_info;
	net_client_ctrl_cmd_info_t *ctrl_cmd_info = (net_client_ctrl_cmd_info_t *)_ctrl_cmd_info;

	switch(ctrl_cmd_info->cmd) {
		case NET_CLIENT_CTRL_CMD_QUERY_ACCOUNT: {
			account_request_info_t *account_request_info = (account_request_info_t *)ctrl_cmd_info->args;
			OS_ASSERT(account_request_info != NULL);
		}
		break;

		default: {
		}
		break;
	}
}

static void request_init(void *ctx)
{
	int i;
	net_client_info_t *net_client_info = (net_client_info_t *)ctx;

	if(net_client_data_ctx == NULL) {
		net_client_data_ctx = (net_client_data_ctx_t *)os_calloc(1, sizeof(net_client_data_ctx_t));
		OS_ASSERT(net_client_data_ctx != NULL);

		net_client_data_ctx->channels_info = get_channels();
		OS_ASSERT(net_client_data_ctx->channels_info != NULL);

		net_client_data_ctx->device_cmd_ctx = (command_status_t *)os_calloc(ARRAY_SIZE(net_client_command_item_device_table), sizeof(command_status_t));
		OS_ASSERT(net_client_data_ctx->device_cmd_ctx != NULL);

		net_client_data_ctx->channel_data_ctx = (net_client_channel_data_ctx_t *)os_calloc(net_client_data_ctx->channels_info->channel_number, sizeof(net_client_channel_data_ctx_t));
		OS_ASSERT(net_client_data_ctx->channel_data_ctx != NULL);

		for(i = 0; i < net_client_data_ctx->channels_info->channel_number; i++) {
			net_client_channel_data_ctx_t *net_client_channel_data_ctx = net_client_data_ctx->channel_data_ctx + i;

			net_client_channel_data_ctx->channel_cmd_ctx = (command_status_t *)os_calloc(ARRAY_SIZE(net_client_command_item_channel_table), sizeof(command_status_t));
			OS_ASSERT(net_client_channel_data_ctx->channel_cmd_ctx != NULL);
		}
	}

	remove_callback(net_client_info->net_client_ctrl_cmd_chain, &net_client_info->net_client_ctrl_cmd_callback_item);
	net_client_info->net_client_ctrl_cmd_callback_item.fn = ocpp_1_6_ctrl_cmd;
	net_client_info->net_client_ctrl_cmd_callback_item.fn_ctx = net_client_info;
	OS_ASSERT(register_callback(net_client_info->net_client_ctrl_cmd_chain, &net_client_info->net_client_ctrl_cmd_callback_item) == 0);
}

static void request_before_create_server_connect(void *ctx)
{
	debug("");
}

static void request_after_create_server_connect(void *ctx)
{
	net_client_info_t *net_client_info = (net_client_info_t *)ctx;
	char key_raw[16];
	char key[16 * 4 / 3 + 4];
	size_t key_size = sizeof(key);
	int ret;
	char *content = "";

	debug("");

	ret = ws_build_key(key_raw, sizeof(key_raw), 1, key, &key_size);

	if(ret != 0) {
		set_client_state(net_client_info, CLIENT_RESET);
		debug("");
	}

	content = "Origin: http://coolaf.com\r\n";

	net_client_info->send_message_buffer.used = ws_build_header((char *)net_client_info->send_message_buffer.buffer,
	        sizeof(net_client_info->send_message_buffer.buffer),
	        net_client_info->net_client_addr_info.host,
	        net_client_info->net_client_addr_info.port,
	        net_client_info->net_client_addr_info.path,
	        key,
	        content);

	_hexdump("request header", (const char *)net_client_info->send_message_buffer.buffer, net_client_info->send_message_buffer.used);
	debug("request header:\n\'%s\'", net_client_info->send_message_buffer.buffer);

	if(poll_wait_write_available(net_client_info->sock_fd, 5000) == 0) {
		ret = net_client_info->protocol_if->net_send(net_client_info, net_client_info->send_message_buffer.buffer, net_client_info->send_message_buffer.used);

		if(ret != net_client_info->send_message_buffer.used) {
			set_client_state(net_client_info, CLIENT_RESET);
			debug("send header failed(%d)!", ret);
		} else {
			debug("send header successful!");
		}
	} else {
		set_client_state(net_client_info, CLIENT_RESET);
	}

	if(poll_wait_read_available(net_client_info->sock_fd, 5000) == 0) {
		ret = net_client_info->protocol_if->net_recv(net_client_info, net_client_info->recv_message_buffer.buffer, sizeof(net_client_info->recv_message_buffer.buffer));

		if(ret <= 0) {
			debug("receive header response failed(%d)!", ret);
			set_client_state(net_client_info, CLIENT_RESET);
		} else {
			debug("receive header response successful!");
			_hexdump("response header", (const char *)net_client_info->recv_message_buffer.buffer, ret);
			debug("response header:\n\'%s\'", net_client_info->send_message_buffer.buffer);

			if(ws_match_response_header((char *)net_client_info->recv_message_buffer.buffer, NULL) != 0) {
				debug("match header response failed!");
				set_client_state(net_client_info, CLIENT_RESET);
			} else {
				debug("match header response successful!");
			}
		}
	} else {
		set_client_state(net_client_info, CLIENT_RESET);
	}
}

static void request_before_close_server_connect(void *ctx)
{
	debug("");
}

static void request_after_close_server_connect(void *ctx)
{
	debug("");
}

static void request_parse(void *ctx, char *buffer, size_t size, size_t max_request_size, char **prequest, size_t *request_size)
{
	char *request = NULL;
	uint8_t fin = 0;
	ws_opcode_t opcode;
	int ret;

	//_hexdump("request_parse", (const char *)buffer, size);

	ret = ws_decode((uint8_t *)buffer, size, (uint8_t **)&request, &size, &fin, &opcode);

	if(ret == 0) {
		debug("fin:%d", fin);
		debug("opcode:%s", get_ws_opcode_des(opcode));
		debug("size:%d", size);
	} else {
		debug("ws_decode failed!");

		if(size >= max_request_size) {
			request = NULL;
		}
	}

	*prequest = request;
	*request_size = size;

	return;
}

static char *get_net_client_cmd_device_des(net_client_device_command_t cmd)
{
	char *des = "unknow";

	switch(cmd) {
			add_des_case(NET_CLIENT_DEVICE_COMMAND_NONE);

		default: {
		}
		break;
	}

	return des;
}

static char *get_net_client_cmd_channel_des(net_client_channel_command_t cmd)
{
	char *des = "unknow";

	switch(cmd) {

			add_des_case(NET_CLIENT_CHANNEL_COMMAND_NONE);

		default: {
		}
		break;
	}

	return des;
}

static void ocpp_1_6_response(void *ctx, uint8_t *request, uint16_t request_size, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	int ret = 0;
	int i;
	int j;
	//ocpp_1_6_frame_header_t *ocpp_1_6_frame_header = (ocpp_1_6_frame_header_t *)request;
	net_client_info_t *net_client_info = (net_client_info_t *)ctx;
	command_status_t *device_cmd_ctx = net_client_data_ctx->device_cmd_ctx;
	net_client_channel_data_ctx_t *channel_data_ctx = net_client_data_ctx->channel_data_ctx;
	uint8_t handled = 0;
	channels_info_t *channels_info = net_client_data_ctx->channels_info;

	for(i = 0; i < ARRAY_SIZE(net_client_command_item_device_table); i++) {
		net_client_command_item_t *item = net_client_command_item_device_table[i];

		if(device_cmd_ctx[item->cmd].available == 1) {
			continue;
		}

		//if(item->frame != ocpp_1_6_frame_header->cmd.cmd) {
		//	continue;
		//}

		net_client_data_ctx->request_timeout = 0;

		if(item->response_callback == NULL) {
			debug("");
			continue;
		}

		ret = item->response_callback(net_client_info, item, 0, request, request_size, send_buffer, send_buffer_size);

		if(ret != 0) {
			if(ret == 1) {//ignore
			} else {
				debug("device cmd %d(%s) response error!", item->cmd, get_net_client_cmd_channel_des(item->cmd));
				handled = 1;
			}
		} else {
			debug("device cmd:%d(%s) response", item->cmd, get_net_client_cmd_device_des(item->cmd));
			handled = 1;
		}
	}

	if(handled == 1) {
		return;
	}

	for(j = 0; j < channels_info->channel_number; j++) {
		command_status_t *channel_cmd_ctx = channel_data_ctx[j].channel_cmd_ctx;

		for(i = 0; i < ARRAY_SIZE(net_client_command_item_channel_table); i++) {
			net_client_command_item_t *item = net_client_command_item_channel_table[i];

			if(channel_cmd_ctx[item->cmd].available == 1) {
				continue;
			}

			//if(item->frame != ocpp_1_6_frame_header->cmd.cmd) {
			//	continue;
			//}

			net_client_data_ctx->request_timeout = 0;

			if(item->response_callback == NULL) {
				debug("");
				continue;
			}

			ret = item->response_callback(net_client_info, item, j, request, request_size, send_buffer, send_buffer_size);

			if(ret != 0) {
				if(ret == 1) {
				} else {
					debug("channel %d cmd %d(%s) response error!", j, item->cmd, get_net_client_cmd_channel_des(item->cmd));
				}
			} else {
				debug("channel %d cmd:%d(%s) response", j, item->cmd, get_net_client_cmd_channel_des(item->cmd));
				handled = 1;
				break;
			}
		}

		if(handled == 1) {
			break;
		}
	}

	return;
}

static void request_process(void *ctx, uint8_t *request, uint16_t request_size, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	//_hexdump("request_process", (const char *)request, request_size);

	ocpp_1_6_response(ctx, request, request_size, send_buffer, send_buffer_size);
}

#define RESPONSE_TIMEOUT_DURATOIN (3 * 1000)

static void ocpp_1_6_periodic(net_client_info_t *net_client_info)
{
	int i;
	int j;
	uint32_t ticks = osKernelSysTick();
	command_status_t *device_cmd_ctx = net_client_data_ctx->device_cmd_ctx;
	net_client_channel_data_ctx_t *channel_data_ctx = net_client_data_ctx->channel_data_ctx;
	channels_info_t *channels_info = net_client_data_ctx->channels_info;

	for(i = 0; i < ARRAY_SIZE(net_client_command_item_device_table); i++) {
		net_client_command_item_t *item = net_client_command_item_device_table[i];

		if(device_cmd_ctx[item->cmd].state == COMMAND_STATE_RESPONSE) {
			if(ticks_duration(ticks, device_cmd_ctx[item->cmd].send_stamp) >= RESPONSE_TIMEOUT_DURATOIN) {
				net_client_data_ctx->request_timeout++;
				debug("device cmd %d(%s) timeout", item->cmd, get_net_client_cmd_device_des(item->cmd));
				device_cmd_ctx[item->cmd].state = COMMAND_STATE_IDLE;

				if(item->timeout_callback != NULL) {
					item->timeout_callback(net_client_info, item, 0);
				}
			}
		}

		if(item->periodic == 0) {
			continue;
		}

		if(device_cmd_ctx[item->cmd].available == 0) {
			continue;
		}

		if(ticks_duration(ticks, device_cmd_ctx[item->cmd].stamp) >= item->periodic) {
			debug("device cmd %d(%s) start", item->cmd, get_net_client_cmd_device_des(item->cmd));
			device_cmd_ctx[item->cmd].state = COMMAND_STATE_REQUEST;
			device_cmd_ctx[item->cmd].stamp = ticks;
		}
	}

	for(j = 0; j < channels_info->channel_number; j++) {
		command_status_t *channel_cmd_ctx = channel_data_ctx[j].channel_cmd_ctx;

		for(i = 0; i < ARRAY_SIZE(net_client_command_item_channel_table); i++) {
			net_client_command_item_t *item = net_client_command_item_channel_table[i];

			if(channel_cmd_ctx[item->cmd].state == COMMAND_STATE_RESPONSE) {
				if(ticks_duration(ticks, channel_cmd_ctx[item->cmd].send_stamp) >= RESPONSE_TIMEOUT_DURATOIN) {
					net_client_data_ctx->request_timeout++;
					debug("channel %d cmd %d(%s) timeout", j, item->cmd, get_net_client_cmd_channel_des(item->cmd));
					channel_cmd_ctx[item->cmd].state = COMMAND_STATE_IDLE;

					if(item->timeout_callback != NULL) {
						item->timeout_callback(net_client_info, item, j);
					}
				}
			}

			if(item->periodic == 0) {
				continue;
			}

			if(channel_cmd_ctx[item->cmd].available == 0) {
				continue;
			}

			if(channel_cmd_ctx[item->cmd].state != COMMAND_STATE_IDLE) {
				continue;
			}

			if(ticks_duration(ticks, channel_cmd_ctx[item->cmd].stamp) >= item->periodic) {
				debug("channel %d cmd %d(%s) start", j, item->cmd, get_net_client_cmd_channel_des(item->cmd));
				channel_cmd_ctx[item->cmd].state = COMMAND_STATE_REQUEST;
				channel_cmd_ctx[item->cmd].stamp = ticks;
			}
		}
	}

	if(net_client_data_ctx->request_timeout >= 10) {
		net_client_data_ctx->request_timeout = 0;
		debug("reset connect!");
		set_client_state(net_client_info, CLIENT_RESET);
	}
}

static void request_process_request(net_client_info_t *net_client_info, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	int i;
	int j;
	int ret;
	command_status_t *device_cmd_ctx = net_client_data_ctx->device_cmd_ctx;
	net_client_channel_data_ctx_t *channel_data_ctx = net_client_data_ctx->channel_data_ctx;
	channels_info_t *channels_info = net_client_data_ctx->channels_info;

	for(i = 0; i < ARRAY_SIZE(net_client_command_item_device_table); i++) {
		uint32_t ticks = osKernelSysTick();

		net_client_command_item_t *item = net_client_command_item_device_table[i];

		if(device_cmd_ctx[item->cmd].state != COMMAND_STATE_REQUEST) {
			continue;
		}

		if(device_cmd_ctx[item->cmd].available == 0) {
			continue;
		}

		device_cmd_ctx[item->cmd].send_stamp = ticks;

		debug("request device cmd:%d(%s)", item->cmd, get_net_client_cmd_device_des(item->cmd));

		if(item->request_callback == NULL) {
			debug("");
			continue;
		}

		memset(send_buffer, 0, send_buffer_size);

		ret = item->request_callback(net_client_info, item, 0, send_buffer, send_buffer_size);

		if(ret != 0) {
			debug("send device request cmd %d(%s) error!", item->cmd, get_net_client_cmd_device_des(item->cmd));
			continue;
		}
	}

	for(j = 0; j < channels_info->channel_number; j++) {
		command_status_t *channel_cmd_ctx = channel_data_ctx[j].channel_cmd_ctx;

		for(i = 0; i < ARRAY_SIZE(net_client_command_item_channel_table); i++) {
			uint32_t ticks = osKernelSysTick();

			net_client_command_item_t *item = net_client_command_item_channel_table[i];

			if(channel_cmd_ctx[item->cmd].state != COMMAND_STATE_REQUEST) {
				continue;
			}

			if(channel_cmd_ctx[item->cmd].available == 0) {
				continue;
			}

			channel_cmd_ctx[item->cmd].send_stamp = ticks;

			debug("request channel %d cmd:%d(%s)", j, item->cmd, get_net_client_cmd_channel_des(item->cmd));

			if(item->request_callback == NULL) {
				debug("");
				continue;
			}

			ret = item->request_callback(net_client_info, item, j, send_buffer, send_buffer_size);

			if(ret != 0) {
				debug("send channel %d request cmd %d(%s) error!", j, item->cmd, get_net_client_cmd_channel_des(item->cmd));
				continue;
			}
		}
	}
}

static void request_periodic(void *ctx, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	uint32_t ticks = osKernelSysTick();
	static uint32_t send_stamp = 0;
	net_client_info_t *net_client_info = (net_client_info_t *)ctx;
	char *s = "xiaofei";

	if(ticks_duration(ticks, send_stamp) < 3000) {
		return;
	}

	send_stamp = ticks;

	debug("%s", get_ocpp_measurand_name(enum_ocpp_measurand4(Energy, Active, Export, Register)));

	if(get_client_state(net_client_info) != CLIENT_CONNECTED) {
		//debug("");
		return;
	}

	memset(send_buffer, 0, send_buffer_size);
	send_frame(net_client_info, (uint8_t *)s, strlen(s), send_buffer, send_buffer_size);
	ocpp_1_6_periodic(net_client_info);
	request_process_request(net_client_info, send_buffer, send_buffer_size);
}

request_callback_t request_callback_ocpp_1_6 = {
	.type = REQUEST_TYPE_OCPP_1_6,
	.init = request_init,
	.before_connect = request_before_create_server_connect,
	.after_connect = request_after_create_server_connect,
	.before_close = request_before_close_server_connect,
	.after_close = request_after_close_server_connect,
	.parse = request_parse,
	.process = request_process,
	.periodic = request_periodic,
};
