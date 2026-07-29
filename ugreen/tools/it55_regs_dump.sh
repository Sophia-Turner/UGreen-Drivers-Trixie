#!/bin/bash

# 定义EC寄存器地址和名称的映射（按地址排序）
declare -A ec_registers=(
    ["0x34"]="Fan1_Speed1"
    ["0x35"]="Fan1_Speed"
    ["0x36"]="Fan2_Speed1"
    ["0x37"]="Fan2_Speed"
    ["0x38"]="Fan3_Speed1"
    ["0x39"]="Fan3_Speed"
    ["0x3A"]="Fan4_Speed1"
    ["0x3B"]="Fan4_Speed"
    ["0x5B"]="Vlot_ADC0"
    ["0x5C"]="Vlot_ADC1"
    ["0x5D"]="Vlot_ADC2"
    ["0x5E"]="Vlot_ADC3"
    ["0x5F"]="Temp_ADC4"
    ["0x60"]="Vlot_ADC5"
    ["0x70"]="EC_CPU_DTS"
    ["0x98"]="WatchDog_Flag"
    ["0x99"]="Lan_Wake_Flag"
    ["0x9A"]="Lan_Wake_G3_Flag"
    ["0x9C"]="Entry_EUP_Flag"
    ["0x9E"]="Beep_Flag"
    ["0xA0"]="Power_Mode_Flag"
    ["0xA1"]="Power_Mode_Cnt"
    ["0xA2"]="WatchDog_Cnt1"
    ["0xA3"]="WatchDog_Cnt"
    ["0xA5"]="BKLT_PWM_Value"
    ["0xB0"]="Fan1_Control"
    ["0xB1"]="Set_Fan1_PWM"
    ["0xB2"]="Fan2_Control"
    ["0xB3"]="Set_Fan2_PWM"
    ["0xB4"]="Fan3_Control"
    ["0xB5"]="Set_Fan3_PWM"
    ["0xB6"]="Fan4_Control"
    ["0xB7"]="Set_Fan4_PWM"
)

# 检查/proc接口是否存在
if [ ! -f /proc/ec_mem ]; then
    echo "错误: /proc/ec_mem 接口不存在，请确保内核模块已加载"
    exit 1
fi

# 调试日志文件
DEBUG_LOG="ec_register_debug.log"
echo "==== EC寄存器读取调试日志 ====" > "$DEBUG_LOG"
echo "开始时间: $(date)" >> "$DEBUG_LOG"

# 打印表头
printf "%-10s %-20s %-10s %s\n" "Address" "Name" "Value" "Status"
echo "--------------------------------------------------"

# 按地址排序后处理
for addr in $(printf '%s\n' "${!ec_registers[@]}" | sort -V); do
    # 转换地址为十进制用于比较
    addr_dec=$((addr))
    
    # 调试输出
    echo "正在处理地址: $addr (${ec_registers[$addr]})" >> "$DEBUG_LOG"
    
    # 检查地址范围 (0x00-0xFF)
    if [ $addr_dec -lt 0 ] || [ $addr_dec -gt 255 ]; then
        printf "%-10s %-20s %-10s %s\n" "$addr" "${ec_registers[$addr]}" "N/A" "地址越界" | tee -a "$DEBUG_LOG"
        continue
    fi
    
    # 发送读取命令
    echo "read $addr" > /proc/ec_mem 2>> "$DEBUG_LOG"
    if [ $? -ne 0 ]; then
        printf "%-10s %-20s %-10s %s\n" "$addr" "${ec_registers[$addr]}" "N/A" "写入失败" | tee -a "$DEBUG_LOG"
        continue
    fi
    
    # 获取完整输出
    output=$(cat /proc/ec_mem 2>> "$DEBUG_LOG")
    echo "$output" >> "$DEBUG_LOG"
    
    # 从输出中提取值
    value=$(echo "$output" | awk '/Last read value:/ {print $NF}')
    if [ -z "$value" ]; then
        value=$(echo "$output" | grep -A1 "Current address" | tail -n1 | awk '{print $1}')
        [ -z "$value" ] && value="N/A"
    fi
    
    # 检查值是否有效
    if [[ "$value" =~ ^0x[0-9A-Fa-f]+$ ]] || [[ "$value" =~ ^[0-9]+$ ]]; then
        status="成功"
    else
        status="无效值"
    fi
    
    # 打印格式化输出
    printf "%-10s %-20s %-10s %s\n" "$addr" "${ec_registers[$addr]}" "$value" "$status" | tee -a "$DEBUG_LOG"
    
    # 小延迟避免EC过载
    sleep 0.1
done

echo "--------------------------------------------------"
echo "所有寄存器读取完成"
echo "详细调试日志已保存到: $DEBUG_LOG"

# 显示最后10行调试信息
echo -e "\n=== 最后10条调试信息 ==="
tail -n 10 "$DEBUG_LOG"