//
//  boardTest.ino
//  触发板临时测试文件
//
//  Created by 张頔 on 2020/5/24.
//  Edition 2020/10/21 v1.1
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
    Serial.begin(9600);
    // digitalWrite(Trig, LOW);

    digitalWrite(LED4, HIGH); // 这4行测试看门狗
    digitalWrite(LED5, HIGH);
    delay(1000);
    digitalWrite(LED5, LOW);
}

unsigned long current_Millis = 0;
bool trig_flag = false;
unsigned long trig_period = 1; // 脉宽长1ms
unsigned long trig_millis = 0;
unsigned long message_delivery_mills = 0;
int value_flux = 0;
int value_voltage = 0;
int trig_EN = LOW;
int num_count = 0;
int jump1_state = LOW;
int frequency_control = 50;

void loop()
{
    trig_millis = millis();
    message_delivery_mills = trig_millis;
    while(true)
    {
        if (!trig_flag)
        {
            current_Millis = millis();
            if (current_Millis - trig_millis >= 20) // 50Hz
            {
                trig_flag = true;
                trig_millis = current_Millis;
                digitalWrite(Trig, HIGH);
            }
            else {
                1;
            }
        }
        else
        {
            current_Millis = millis();
            if (current_Millis - trig_millis >= trig_period)
            {
                trig_flag = false;
                digitalWrite(Trig, LOW);
                num_count += 1;
            }
            else {
                1;
            }
        }
        // 调理电路
        current_Millis = millis();
        if (current_Millis - message_delivery_mills >= 100)
        {
            message_delivery_mills = current_Millis;
            value_flux = analogRead(AD1);
            value_voltage = analogRead(AD2);
            Serial.print("AD1和AD2口读取的值分别是：");
            Serial.print(value_flux);
            Serial.print("\t"); // 这句话可能会出问题
            Serial.print(value_voltage);
            Serial.println();
        }
        else
        {
            1;
        }
        // Enable
        jump1_state = digitalRead(Jump1);
        if (jump1_state == LOW)
            frequency_control = 50;
        else
            frequency_control = 25;
        if (num_count >= frequency_control)
        {
            num_count = 0;
            if (trig_EN == LOW)
            {
                trig_EN = HIGH;
                digitalWrite(Enable, trig_EN);
            }
            else
            {
                trig_EN = LOW;
                digitalWrite(Enable, trig_EN);
            }
            
        }
    }
}
