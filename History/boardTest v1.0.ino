//
//  boardTest.ino
//  毕设触发板临时测试文件
//
//  Created by 张頔 on 2020/4/28.
//  Edition 2020/5/25 v1.2.2
//  Copyright © 2020年 CIEE. All rights reserved.
//


// Arduino功能区，分别是喂狗信号、触发使能信号、触发信号和过零点
// D6 = WDI (9)
// D7 = TrigEN
// D8 = Trig1
// D9 = U_Zero
int WDI = 6;
int Enable = 7;
int Trig = 8;
int Zero = 9;

// 调理电路，AD1和AD2分别可作输出脉冲电压和流量计的采集信息输入
// A0 = AD1 (27)
// A1 = AD2 (26)
int AD1 = 14; // 采集电压
int AD2 = 15; // 采集流量

// 切换到不同的工作状态
// D10 = Jump1 (13)
// D11 = Jump2
// D12 = Jump3
int Jump1 = 10;
int Jump2 = 11;
int Jump3 = 12;

// 代表不同的工作状态，LED4和LED5分别代表220V电源输入是否正常
// LED4 = D2 (5)
// LED3 = D3 (6)
// LED2 = D4
// LED1 = D5
// LED5 = D13 (30)
int LED4 = 2;
int LED3 = 3;
int LED2 = 4;
int LED1 = 5;
int LED5 = 13;

void setup()
{
    pinMode(WDI, OUTPUT);
    pinMode(Enable, OUTPUT);
    pinMode(Trig, OUTPUT);
    pinMode(Zero, INPUT);

    pinMode(AD1, INPUT);
    pinMode(AD2, INPUT);

    pinMode(Jump1, INPUT);
    pinMode(Jump2, INPUT);
    pinMode(Jump3, INPUT);

    pinMode(LED4, OUTPUT);
    pinMode(LED3, OUTPUT);
    pinMode(LED2, OUTPUT);
    pinMode(LED1, OUTPUT);
    pinMode(LED5, OUTPUT);
    // Serial.begin(9600);
    // digitalWrite(Trig, LOW);
}

unsigned long current_Millis = 0;
bool trig_flag = false;
unsigned long trig_period = 5;
unsigned long trig_millis = 0;

void loop()
{
    while(true)
    {
        if (!trig_flag)
        {
            current_Millis = millis();
            if (current_Millis - trig_millis >= 20) // 半秒一个信号
            {
                trig_flag = true;
                trig_millis = current_Millis;
                digitalWrite(Trig, HIGH);
            }
            else {
                continue;
            }
        }
        else
        {
            current_Millis = millis();
            if (current_Millis - trig_millis >= trig_period)
            {
                trig_flag = false;
                digitalWrite(Trig, LOW);
            }
            else {
                continue;
            }
        }
    }
}
