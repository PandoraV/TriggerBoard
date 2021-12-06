//
//  main.ino
//  毕设触发板
//
//  Created by 张頔 on 2020/4/28.
//  Edition 2021/12/06 v1.5.1
//  Copyright © 2020年 CIEE. All rights reserved.
//
//  


// Arduino功能区，分别是喂狗信号、触发使能信号、触发信号和过零点
int WDI = 6;
int Enable = 7;
int Trig = 8;
int Zero = 9;

// 调理电路，AD1和AD2分别可作输出脉冲电压和流量计的采集信息输入
int AD1 = 14; // 采集电压
int AD2 = 15; // 采集流量

// 切换到不同的工作状态
int Jump1 = 10;
int Jump2 = 11;
int Jump3 = 12;

// 代表不同的工作状态，LED4和LED5分别代表220V电源输入是否正常
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

    pinMode(Jump1, INPUT);
    pinMode(Jump2, INPUT);
    pinMode(Jump3, INPUT);

    pinMode(LED4, OUTPUT);
    pinMode(LED3, OUTPUT);
    pinMode(LED2, OUTPUT);
    pinMode(LED1, OUTPUT);
    pinMode(LED5, OUTPUT);

    digitalWrite(Trig, LOW);
}

int trig1 = LOW; // Trig输出信号
int trig_EN = LOW; // Enable输出信号
int WDI_state = LOW; // WDI输出信号
int Jump1_state = LOW; // 拨码开关1
int Jump2_state = LOW; // 拨码开关2
int Jump3_state = LOW; // 拨码开关3
int judge_edge = HIGH; // 判断为HIGH的时候，就是在正半周触发；LOW的时候，就是负半周触发
int num_count = 0; // 50Hz频率周期计数
int work_state = 0; //Jump状态

bool zero_flag = false; // 电压过零点是否已记录
bool first_zero_flag = false; // 第一次电压过阈值是否记录
bool silence = false; // 溢出后周期内保持沉默
bool delay_angle_flag = false; // 是否延迟触发
bool trig_flag = false; // 本次电压过零是否已经触发
bool during_delay_flag = false; // 是否处于延迟角中
bool frequency_input_normal = true; // 输入的50Hz信号是否在正常工作

unsigned long current_Millis = 0; // 当期时间
unsigned long voltage_first_zero_millis = 0; // 电压过零点出现时间
unsigned long trig_start_micros = 0; // 触发开始时间
unsigned long WDI_delivery_mills = 0; // 上次喂狗信号
unsigned long voltage_reco_zero_millis = 0; // 电压过零点记录时间

unsigned long trig_period = 300; // 触发信号脉宽为300μs
unsigned long voltage_Zero_Judge_Period = 2; // 过零点信号脉宽长达2ms
unsigned long WDI_period = 1; // 喂狗信号脉宽暂定1ms
unsigned long no_WDI_period = 1500; // 每1.5s发一次喂狗信号，该芯片需1.6s翻转一次

unsigned long delay_angle_period = 2500; // 2.5ms，45度角
unsigned long delay_angle_start = 0; // 触发延时计时开始
unsigned long current_micros = 0; // 微秒级计时

void overFlow()
{
    // 触发判断溢出时，应当把所有状态清零
    
    // 触发
    if (trig1 == HIGH)
    {
        trig1 = LOW;
        digitalWrite(Trig, trig1);
    }

    // 喂狗
    if (WDI_state == HIGH)
    {
        delay(1);
        WDI_state = LOW;
    }
    else 
    {
        digitalWrite(WDI, HIGH);
        delay(1);
        digitalWrite(WDI, LOW);
    }

    // 标志位
    first_zero_flag = false;
    zero_flag = false;
    silence = true;
    trig_flag = false;
}

void micro_overFlow(int type)
{
    // 给触发信号的时候溢出
    switch (type)
    {
        case 1:
        {
            delayMicroseconds(110);
            trig1 = LOW;
            digitalWrite(Trig, trig1);
            break;
        }
        case 2:
        {
            zero_flag = false;
            break;
        }
        default:
            break;
    }
}

void frequency_division() // 分频函数
{
    bool leap = false;      // 分频标志位
    if (work_state == 0)    // 当分频的workstate为0的时候
        leap = true;        // 恒不触发
    else if (work_state == 1) // 1分频，即不改变初始频率
        1;
    else if (num_count % work_state != 0) // n分频
    {
        leap = true;
    }
    else
    {
        num_count = 0;
        leap = false;
    }
    
    if (!leap) // leap为false，即num_count为work_state的整倍数时，使能开启
    {
        if (trig_EN == HIGH)
        {
            trig_EN = LOW;
        }
        digitalWrite(Enable, trig_EN);
    }
    else 
    {
        if (trig_EN == LOW)
        {
            trig_EN = HIGH;
        }
        digitalWrite(Enable, trig_EN);
    }
    
}

void WDI_deliver()
{
    if (WDI_state == LOW)
    {
        // 没有给喂狗信号
        current_Millis = millis();
        if (current_Millis - WDI_delivery_mills >= no_WDI_period)
        {
            WDI_state = HIGH;
            WDI_delivery_mills = current_Millis;
            digitalWrite(WDI, WDI_state);
        }
        else if (current_Millis < WDI_delivery_mills)
        {
            overFlow();
        }
    }
    else
    {
        // 正在给喂狗信号
        current_Millis = millis();
        if (current_Millis - WDI_delivery_mills >= WDI_period)
        {
            WDI_state = LOW;
            digitalWrite(WDI, WDI_state);
        }
        else if (current_Millis < WDI_delivery_mills)
        {
            overFlow();
        }
    }
}

void loop()
{
    // 程序开始运行时先给出喂狗信号
    digitalWrite(WDI, HIGH);
    delayMicroseconds(1000);
    digitalWrite(WDI, LOW);
    WDI_delivery_mills = millis();

    digitalWrite(Enable, LOW);
    digitalWrite(Trig, LOW);
    digitalWrite(WDI, LOW);
    digitalWrite(LED4, LOW); // 正常时LED4亮，5灭
    digitalWrite(LED5, LOW);

    while(true)
    {
        Jump1_state = digitalRead(Jump1);
        Jump2_state = digitalRead(Jump2);
        Jump3_state = digitalRead(Jump3);
        work_state = 4*Jump1_state + 2*Jump2_state + 1*Jump3_state;

        if (digitalRead(Zero) == judge_edge)
        {
            if (silence) // 保持静默直到电压水平恢复
            {
                continue;
            }
            if (!trig_flag)
                if (!first_zero_flag) // 第一次超过电压阈值
                {
                    first_zero_flag = true;
                    voltage_first_zero_millis = millis();
                }
                else if (!zero_flag) // 是否记录电压过零点
                {
                    current_Millis = millis();
                    if (current_Millis - voltage_first_zero_millis >= voltage_Zero_Judge_Period)
                    {
                        voltage_reco_zero_millis = current_Millis;
                        zero_flag = true; // 记录到过零点
                        frequency_input_normal = true; // 能检测到过零点表明50Hz正常
                    }
                    else if (current_Millis < voltage_Zero_Judge_Period) // 溢出
                    {
                        overFlow();
                        continue;
                    }
                }
            else if (!zero_flag)
            {
                // 如果trig_flag为真，即已经进入触发判断，但zero_flag又为假，即不再进入触发判断
                // 则此时为已经给过触发脉冲之后的时间
                // 该程序用以补全50Hz判断逻辑在长时间处于低电平下检测
                current_Millis = millis();
                if (current_Millis - voltage_first_zero_millis >= 25)
                {
                    frequency_input_normal = false;
                }
                else if (current_Millis < voltage_Zero_Judge_Period) // 溢出
                {
                    overFlow();
                    continue;
                }
                // 恢复LED指示
                if (frequency_input_normal) {
                    digitalWrite(LED4, HIGH);
                    digitalWrite(LED5, LOW);
                }
                else {
                    digitalWrite(LED4, LOW);
                    digitalWrite(LED5, HIGH);
                }
            }
        }
        else
        {
            // 50Hz工作检测电路
            // 这里只能够判断触发为上升沿的时候，即为高电平的时候，判断逻辑不适用于持续的低电平
            current_Millis = millis();
            if (current_Millis - voltage_first_zero_millis >= 25)
            {
                frequency_input_normal = false;
            }
            else if (current_Millis < voltage_Zero_Judge_Period) // 溢出
            {
                overFlow();
                continue;
            }
            // 标志位清零
            zero_flag = false;
            first_zero_flag = false;
            trig_flag = false;
            silence = false;
            // 恢复LED指示
            if (frequency_input_normal) {
                digitalWrite(LED4, HIGH);
                digitalWrite(LED5, LOW);
            }
            else {
                digitalWrite(LED4, LOW);
                digitalWrite(LED5, HIGH);
            }
        }
        if (zero_flag) // 如果电压过零点已经被记录
        {
            trig_flag = true; // 不再记录过零点操作
            if (trig1 == LOW) // 还没在触发状态
            {
                // 设置触发角
                if (!delay_angle_flag) // 无延迟导通
                {
                    trig1 = HIGH;
                    trig_start_micros = micros();
                    digitalWrite(Trig, trig1);
                }
                else // 开始延迟
                {
                    if (!during_delay_flag)
                    {
                        during_delay_flag = true;
                        delay_angle_start = micros();
                    }
                    else
                    {
                        current_micros = micros();
                        if (current_micros - delay_angle_start >= delay_angle_period)
                        {
                            trig1 = HIGH;
                            during_delay_flag = false;
                            trig_start_micros = micros();
                            digitalWrite(Trig, trig1);
                        }
                        else if (current_micros < delay_angle_start) // 溢出
                        {
                            micro_overFlow(2);
                        }
                    }
                }
            }
            else // 在触发了
            {
                current_micros = micros();
                if (current_micros - trig_start_micros >= trig_period)
                {
                    zero_flag = false; // 不再进入触发判断
                    trig1 = LOW;
                    digitalWrite(Trig, trig1);
                    num_count += 1;
                    frequency_division();
                }
                else if (current_micros < trig_start_micros) // 溢出
                {
                    micro_overFlow(1);
                }
            }
        }
        WDI_deliver();
    }
}