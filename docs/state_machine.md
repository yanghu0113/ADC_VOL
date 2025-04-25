# 充电状态机图

```mermaid
stateDiagram-v2
    [*] --> INIT
    INIT --> IDLE: 初始化完成
    IDLE --> CONNECTED: CP 12V→9V\n且PP有效
    CONNECTED --> CHARGING_REQ: CP 9V→6V
    CHARGING_REQ --> CHARGING: 闭合接触器
    CHARGING --> CONNECTED: CP 6V→9V\n断开接触器
    CONNECTED --> IDLE: CP 9V→12V
    IDLE --> FAULT: CP异常\n(E/F状态)
    CONNECTED --> FAULT: CP异常\n(E/F状态)
    CHARGING --> FAULT: CP异常\n(E/F状态)
    FAULT --> IDLE: CP恢复12V
