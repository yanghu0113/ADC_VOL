# 充电时序图

```mermaid
sequenceDiagram
    participant Vehicle
    participant EVSE
    Note over EVSE: 初始状态(12V)
    Vehicle->>EVSE: 连接(CP 12V→9V)
    EVSE->>Vehicle: 检测PP电阻
    EVSE->>Vehicle: 发送PWM电流信号
    Vehicle->>EVSE: 请求充电(CP 9V→6V)
    EVSE->>EVSE: 闭合接触器
    EVSE->>Vehicle: 开始供电
    loop 充电过程
        EVSE->>Vehicle: 持续供电
        EVSE->>EVSE: 监测电流/电压
    end
    Vehicle->>EVSE: 停止充电(CP 6V→9V)
    EVSE->>EVSE: 断开接触器
    Vehicle->>EVSE: 断开连接(CP 9V→12V)
