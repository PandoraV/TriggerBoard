//
//  main.ino
//  毕设触发板
//
//  Created by 张頔 on 2020/4/28.
//  Edition 2020/10/21 v1.4
//  Copyright © 2020年 CIEE. All rights reserved.
//
//  触发上升沿可能有问题


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

    // pinMode(A2, OUTPUT);
    // pinMode(A3, OUTPUT);
    // pinMode(A4, OUTPUT);
    // pinMode(A5, OUTPUT);
    // Serial.begin(9600);
    digitalWrite(Trig, LOW);
}

int trig1 = LOW; // Trig输出信号
int trig_EN = LOW; // Enable输出信号
int WDI_state = LOW; // WDI输出信号
int Jump1_state = LOW;
int Jump2_state = LOW;
int Jump3_state = LOW;
int judge_edge = LOW; // 判断为HIGH的时候，就是在负半周触发；LOW的时候，就是正半周触发
int num_count = 0; // 50Hz频率周期计数
int work_state = 0; //Jump状态

bool zero_flag = false; // 电压过零点是否已记录
bool first_zero_flag = false; // 第一次电压过低是否记录
bool silence = false; // 溢出后周期内保持沉默
bool delay_angle_flag = false; // 是否延迟触发
bool trig_flag = false; // 本次电压过零是否已经触发
bool during_delay_flag = false; // 是否处于延迟角中

unsigned long current_Millis = 0; // 当期时间
unsigned long voltage_first_zero_millis = 0; // 电压过零点出现时间
unsigned long trig_start_micros = 0; // 触发开始时间
unsigned long WDI_delivery_mills = 0; // 上次喂狗信号
unsigned long voltage_reco_zero_millis = 0; // 电压过零点记录时间
unsigned long enable_change_millis = 0; // 使能信号切换时间

unsigned long trig_period = 115; // 触发信号脉宽暂定115μs
unsigned long no_trig_period = 115; // 相邻触发信号间隔暂定115μs
unsigned long voltage_Zero_Judge_Period = 1; // 过零点信号脉宽长达暂定1ms
unsigned long WDI_period = 1; // 喂狗信号脉宽暂定1ms
unsigned long no_WDI_period = 99; // 每100ms发一次喂狗信号
unsigned long enable_period = 200; // 占空比为每400ms里的200ms
unsigned long disable_period = 200; // 占空比间隔200ms

unsigned long delay_angle_period = 2500; // 2.5ms，45度角
unsigned long delay_angle_start = 0; // 触发延时计时开始
unsigned long current_micros = 0; // 微秒级计时

void overFlow()
{
    // 触发判断溢出时，应当把所有状态清零
    
    // 触发
    trig1 = LOW;
    digitalWrite(Trig, trig1);

    // 喂狗
    digitalWrite(WDI, HIGH);
    delayMicroseconds(1000);
    digitalWrite(WDI, LOW);
    WDI_delivery_mills = millis();
    WDI_state = LOW;

    // 电压过零
    first_zero_flag = false;
    zero_flag = false;
    silence = true;
}

void micro_overFlow(int type)
{
    // 给触发信号的时候溢出
    // TODO 
    switch (type)
    {
        case 1:
        {
            zero_flag = false; // 不再进入触发判断
            delayMicroseconds(110);
            trig1 = LOW;
            digitalWrite(Trig, trig1);
            break;
        }
        case 2:
        {
            trig1 = LOW;
            digitalWrite(Trig, trig1);
            zero_flag = false;
            break;
        }
        default:
            break;
    }
}

void frequency_division()
{
    bool leap = false;
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
    
    if (!leap)
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
    }
}

void loop()
{
    // 程序开始运行时先给出喂狗信号
    digitalWrite(WDI, HIGH);
    delayMicroseconds(1000);
    digitalWrite(WDI, LOW);
    WDI_delivery_mills = millis();
    enable_change_millis = WDI_delivery_mills;

    digitalWrite(Enable, LOW);
    digitalWrite(Trig, LOW);
    digitalWrite(WDI, LOW);
    digitalWrite(LED4, HIGH);
    digitalWrite(LED5, LOW);

    digitalWrite(LED1, LOW); // 
    digitalWrite(LED2, LOW);
    digitalWrite(LED3, LOW);
    while(true)
    {
        Jump1_state = digitalRead(Jump1);
        Jump2_state = digitalRead(Jump2);
        Jump3_state = digitalRead(Jump3);
        digitalWrite(LED1, Jump1_state);
        digitalWrite(LED2, Jump2_state);
        digitalWrite(LED3, Jump3_state);
        work_state = 4*Jump1_state + 2*Jump2_state + 1*Jump3_state;

        // if (Jump3_state == HIGH && Jump1_state == HIGH && Jump2_state == LOW)
        // {
        //     // TODO
        //     // 根据流量调解频率
        //     // int flux = analogRead(AD2);
        //     current_Millis = millis();
        //     frequency_division();
        //     // if (trig_EN == HIGH)
        //     // {
        //     //     if (current_Millis - enable_change_millis >= disable_period)
        //     //     {
        //     //         trig_EN = LOW;
        //     //         enable_change_millis = current_Millis;
        //     //     }
        //     //     else if (current_Millis < enable_change_millis)
        //     //     {
        //     //         trig_EN = LOW;
        //     //         enable_change_millis = current_Millis;
        //     //     }
        //     // }
        //     // else
        //     // {
        //     //     if (current_Millis - enable_change_millis >= enable_period)
        //     //     {
        //     //         trig_EN = HIGH;
        //     //         enable_change_millis = current_Millis;
        //     //     }
        //     //     else
        //     //     {
        //     //         trig_EN = HIGH;
        //     //         enable_change_millis = current_Millis;
        //     //     }                
        //     // }
        //     trig_EN = LOW;
        //     digitalWrite(Enable, trig_EN);
        //     if (delay_angle_flag)
        //     {
        //         delay_angle_flag = false;
        //     }
        // }
        // else if (Jump2_state == HIGH && Jump1_state == HIGH && Jump3_state == LOW)
        // {
        //     if (trig_EN == HIGH)
        //     {
        //         trig_EN = LOW;
        //     }
        //     digitalWrite(Enable, trig_EN);
        //     if (!delay_angle_flag) // 控制触发角
        //     {
        //         delay_angle_flag = true;
        //     }
        // }
        // else // 优先级为3>2>1, 1为正常工作状态
        // {
        //     if (trig_EN == HIGH)
        //     {
        //         trig_EN = LOW; // TODO
        //     }
        //     digitalWrite(Enable, trig_EN);
        //     if (delay_angle_flag)
        //     {
        //         delay_angle_flag = false;
        //     }
        // }

        if (digitalRead(Zero) == judge_edge)
        {
            if (silence) // 保持静默直到电压水平恢复
            {
                WDI_deliver();
                continue;
            }
            if (!trig_flag)
                if (!first_zero_flag) // 第一次低于电压阈值
                {
                    first_zero_flag = true;
                    voltage_first_zero_millis = millis();
                }
                else if (!zero_flag) // 是否记录电压过零点
                {
                    // 配合分频函数 TODO
                    current_Millis = millis();
                    if (current_Millis - voltage_first_zero_millis >= voltage_Zero_Judge_Period)
                    {
                        voltage_reco_zero_millis = current_Millis;
                        zero_flag = true;
                    }
                    else if (current_Millis < voltage_Zero_Judge_Period)
                    {
                        // 溢出
                        overFlow();
                        continue;
                    }
                }
                else // 是否能检测到电压过零点
                {
                    current_Millis = millis();
                    if (current_Millis - voltage_first_zero_millis >= 10)
                    {
                        digitalWrite(LED4, LOW);
                        digitalWrite(LED5, HIGH);
                    }
                    else if (current_Millis < voltage_Zero_Judge_Period)
                    {
                        // 溢出
                        overFlow();
                        continue;
                    }
                }
            else
            {
                1;
            }
        }
        else
        {
            zero_flag = false;
            first_zero_flag = false;
            trig_flag = false;
            during_delay_flag = false;
            if (silence)
            {
                silence = false;
            }
            digitalWrite(LED4, HIGH);
            digitalWrite(LED5, LOW);
        }
        if (zero_flag) // 如果电压过零点已经被记录
        {
            trig_flag = true; // 不再记录过零点操作
            if (trig1 == LOW) // 还没在触发状态
            {
                // 这里可以添加设置触发角的代码
                if (!delay_angle_flag)
                {
                    trig1 = HIGH;
                    trig_start_micros = micros();
                    digitalWrite(Trig, trig1);
                }
                else
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
                            trig_start_micros = micros();
                            digitalWrite(Trig, trig1);
                        }
                        else if (current_micros < delay_angle_start)
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
                else if (current_micros < trig_start_micros)
                {
                    // 溢出
                    micro_overFlow(1);
                }
            }
        }
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
            else if(current_Millis < WDI_delivery_mills)
            {
                // 溢出
                overFlow();
                continue;
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
                // 溢出
                overFlow();
                continue;
            }
        }
    }
}
