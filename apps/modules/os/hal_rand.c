

/*================================================================
 *   
 *   
 *   文件名称：hal_rand.c
 *   创 建 者：肖飞
 *   创建日期：2021年07月10日 星期六 00时04分24秒
 *   修改日期：2021年07月10日 星期六 00时06分24秒
 *   描    述：
 *
 *================================================================*/

void __wrap_srand(unsigned int seed)
{
}

int __wrap_rand(void)
{
	return HAL_RNG_GetRandomNumber(&hrng);
}

