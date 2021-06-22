

/*================================================================
 *
 *
 *   文件名称：energy_meter_handler_ac_hlw8032.c
 *   创 建 者：肖飞
 *   创建日期：2021年06月21日 星期一 11时09分49秒
 *   修改日期：2021年06月22日 星期二 10时27分04秒
 *   描    述：
 *
 *================================================================*/
#include "energy_meter_handler_ac_hlw8032.h"

#include "uart_data_task.h"

#include "log.h"

#pragma pack(push, 1)

//1、 当 State REG = 0xaa 时，芯片误差修正功能失效，此时电压参数寄存器、电流参数寄存器和
//功率参数寄存器不可用;
//2、 当 State REG = 0x55 时，芯片误差修正功能正常，此时电压参数寄存器、电流参数寄存器和
//功率参数寄存器可用,且电压寄存器、电流寄存器和功率寄存器未溢出;
//3、 当 State REG = 0xfx 时，芯片误差修正功能正常, 此时电压参数寄存器、电流参数寄存器和功
//率参数寄存器可用, State REG 的相应位为 1 时表示相应的寄存器溢出，溢出表示电流、电压
//或功率值非常小，接近 0;
typedef struct {
	uint8_t bit0 : 1;//电压参数寄存器、电流参数寄存器和功率参数器寄存器状态位; 0: 正常 1: 电压参数寄存器、电流参数寄存器和功率参数器寄存器不能使用
	uint8_t bit1 : 1;//功率寄存器状态位;0: 正常 1: 功率寄存器溢出
	uint8_t bit2 : 1;//电流寄存器状态位;0: 正常 1: 电流寄存器溢出
	uint8_t bit3 : 1;//电压寄存器状态位;0: 正常 1: 电压寄存器溢出
	uint8_t bit4 : 1;//保留;1，默认值
	uint8_t bit5 : 1;//保留;1，默认值
	uint8_t bit6 : 1;//保留;1，默认值
	uint8_t bit7 : 1;//保留;1，默认值
} hlw8032_regs_state_t;

//1、 当 bit6 = 1 时，表示电压寄存器的数据状态己更新;
//2、 当 bit5 = 1 时，表示电流寄存器的数据状态己更新;
//3、 当 bit4 = 1 时，表示功率寄存器的数据状态己更新;
typedef struct {
	uint8_t bit0 : 1;//保留
	uint8_t bit1 : 1;//保留
	uint8_t bit2 : 1;//保留
	uint8_t bit3 : 1;//保留
	uint8_t bit4 : 1;//功率寄存器状态标志位;0:功率寄存器数据未更新完成 1:功率寄存器数据己更新
	uint8_t bit5 : 1;//电流寄存器状态标志位;0:电流寄存器数据未更新完成 1:电流寄存器数据己更新
	uint8_t bit6 : 1;//电压寄存器状态标志位;0:电压寄存器数据未更新完成 1:电压寄存器数据己更新
	uint8_t bit7 : 1;//PF 寄存器进位标志位;当 PF 寄存器溢出时，bit7 取反一次
} hlw8032_regs_data_update_t;

typedef struct {
	uint8_t state;//hlw8032_regs_state_t
	uint8_t check_reg;//0x5a, 默认值
	uint8_t voltage_parameter_2;//此寄存器是默认值
	uint8_t voltage_parameter_1;//此寄存器是默认值
	uint8_t voltage_parameter_0;//此寄存器是默认值
	uint8_t voltage_2;
	uint8_t voltage_1;
	uint8_t voltage_0;
	uint8_t current_parameter_2;//此寄存器是默认值
	uint8_t current_parameter_1;//此寄存器是默认值
	uint8_t current_parameter_0;//此寄存器是默认值
	uint8_t current_2;
	uint8_t current_1;
	uint8_t current_0;
	uint8_t power_parameter_2;//此寄存器是默认值
	uint8_t power_parameter_1;//此寄存器是默认值
	uint8_t power_parameter_0;//此寄存器是默认值
	uint8_t power_2;
	uint8_t power_1;
	uint8_t power_0;
	uint8_t data_update;//hlw8032_regs_data_update_t
	//PF 寄存器用来累计脉冲信号，当 16 位寄存器数据溢出时，数据更新寄存器(Data Updata REG)的 bit7 位会进行一次取反,PF 寄存器(PF REG)清零。
	uint8_t pf_1;
	uint8_t pf_0;
	uint8_t crc;//CHECKSUM 除状态寄存器(State REG)、检测寄存器(Check REG)和校验和寄存器(CheckSum REG)之外的寄存器的数据之和的低 8bit
} hlw8032_regs_t;

#pragma pack(pop)

#define UART_BUFFER_SIZE 32
#define VOLTAGE_COEFFICIENT 1.88
#define CURRENT_COEFFICIENT 1
#define POWER_COEFFICIENT 0x34630b8a000
//pulse_number_per_kwh = POWER_COEFFICIENT * VOLTAGE_COEFFICIENT * CURRENT_COEFFICIENT / power_parameter = 0x627cc3c6000 / power_parameter

typedef struct {
	uint8_t state;
	uint32_t stamps;
	uint8_t rx_buffer[UART_BUFFER_SIZE];
	size_t rx_size;
	uint16_t pf_n;//n 是上电时PF 寄存器(PF REG)的寄存器值
	uint8_t pf_c;//c PF 寄存器进位标志位;当 PF 寄存器溢出时，取反一次
	uint32_t pf_count;//PF取反次数
} energy_meter_handler_ctx_t;

static uint8_t calc_hlw_crc(uint8_t *data, size_t size)
{
	int i;
	uint8_t crc = 0;

	for(i = 0; i < size; i++) {
		crc += data[i];
	}

	return crc;
}

static void uart_data_request(void *fn_ctx, void *chain_ctx)
{
	energy_meter_info_t *energy_meter_info = (energy_meter_info_t *)fn_ctx;
	uart_data_task_info_t *uart_data_task_info = (uart_data_task_info_t *)chain_ctx;
	energy_meter_handler_ctx_t *energy_meter_handler_ctx = (energy_meter_handler_ctx_t *)energy_meter_info->ctx;
	channel_info_t *channel_info = energy_meter_info->channel_info;

	energy_meter_handler_ctx->rx_size = uart_rx_data(uart_data_task_info->uart_info, energy_meter_handler_ctx->rx_buffer, UART_BUFFER_SIZE, 100);

	if(energy_meter_handler_ctx->rx_size == sizeof(hlw8032_regs_t)) {
		hlw8032_regs_t *hlw8032_regs = (hlw8032_regs_t *)energy_meter_handler_ctx->rx_buffer;
		u_uint8_bits_t *u_uint8_bits = (u_uint8_bits_t *)&hlw8032_regs->data_update;

		if(hlw8032_regs->crc == calc_hlw_crc(&hlw8032_regs->voltage_parameter_2, &hlw8032_regs->crc - &hlw8032_regs->voltage_parameter_2)) {
		} else {
			debug("crc error!");
		}

		switch(energy_meter_handler_ctx->state) {
			case 0: {
				energy_meter_handler_ctx->pf_n = get_u16_from_u8_lh(hlw8032_regs->pf_0, hlw8032_regs->pf_1);
				energy_meter_handler_ctx->pf_c = u_uint8_bits->s.bit7;
				energy_meter_handler_ctx->state = 1;
			}
			break;

			case 1: {
				if(energy_meter_handler_ctx->pf_c != u_uint8_bits->s.bit7) {
					energy_meter_handler_ctx->pf_c = u_uint8_bits->s.bit7;
					energy_meter_handler_ctx->pf_count++;
				}
			}
			break;

			default: {
			}
			break;
		}

		{
			uint32_t power_parameter = get_u32_from_u8_b0123(hlw8032_regs->power_parameter_0, hlw8032_regs->power_parameter_1, hlw8032_regs->power_parameter_2, 0);
			uint64_t pulse_number_per_kwh;
			uint64_t pf_count;

			pulse_number_per_kwh = 0x627cc3c6000;
			pulse_number_per_kwh /= power_parameter;
			pf_count = energy_meter_handler_ctx->pf_count;
			pf_count = pf_count << 16;
			pf_count += get_u16_from_u8_lh(hlw8032_regs->pf_0, hlw8032_regs->pf_1);
			pf_count *= 10;//0.1kwh
			channel_info->total_energy = pf_count / pulse_number_per_kwh;

			channel_info->voltage = get_u32_from_bcd_b0123(hlw8032_regs->voltage_0, hlw8032_regs->voltage_1, hlw8032_regs->voltage_2, 0);
			channel_info->current = get_u32_from_bcd_b0123(hlw8032_regs->current_0, hlw8032_regs->current_1, hlw8032_regs->current_2, 0);
		}
	}
}

static int handle_init_ac_hlw8032(void *_energy_meter_info)
{
	int ret = 0;
	energy_meter_info_t *energy_meter_info = (energy_meter_info_t *)_energy_meter_info;
	channel_info_t *channel_info = energy_meter_info->channel_info;
	channel_config_t *channel_config = channel_info->channel_config;
	uart_data_task_info_t *uart_data_task_info;

	energy_meter_handler_ctx_t *energy_meter_handler_ctx = (energy_meter_handler_ctx_t *)os_calloc(1, sizeof(energy_meter_handler_ctx_t));
	OS_ASSERT(energy_meter_handler_ctx != NULL);

	energy_meter_info->ctx = energy_meter_handler_ctx;

	uart_data_task_info = get_or_alloc_uart_data_task_info(channel_config->energy_meter_config.huart_energy_meter);
	OS_ASSERT(uart_data_task_info != NULL);

	energy_meter_info->uart_data_request_cb.fn = uart_data_request;
	energy_meter_info->uart_data_request_cb.fn_ctx = energy_meter_info;
	add_uart_data_task_info_cb(uart_data_task_info, &energy_meter_info->uart_data_request_cb);

	return ret;
}

energy_meter_handler_t energy_meter_handler_ac_hlw8032 = {
	.energy_meter_type = CHANNEL_ENERGY_METER_TYPE_AC_HLW8032,
	.handle_init = handle_init_ac_hlw8032,
};
