// 
// 
// 

#include "Program.h"

Program::Program(uint8_t* posArray, bool grip)
{
	a1 = posArray[0];
	a2 = posArray[1];
	a3 = posArray[2];
	a4 = posArray[3];
	a5 = posArray[4];
	a6 = posArray[5];
}

uint8_t Program::getA1()
{
	return a1;
}
uint8_t Program::getA2()
{
	return a2;
}
uint8_t Program::getA3()
{
	return a3;
}
uint8_t Program::getA4()
{
	return a4;
}
uint8_t Program::getA5()
{
	return a5;
}
uint8_t Program::getA6()
{
	return a6;
}
bool Program::getGrip()
{
	return grip;
}
