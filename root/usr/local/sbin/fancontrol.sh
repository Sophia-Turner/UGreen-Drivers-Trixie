#!/bin/bash
#
# CPU Fan Control
#

echo "set 255" > /proc/it86/fan
echo "cpu 255" > /proc/it86/fan

# CONFIGURATION
# Exponential curve (curve steepness) see b=0.045 below
# b=0.02 ~ 0.08

#MIN_PWM=30    # To get the fan moving
MIN_PWM=85
MAX_PWM=255

T_MIN=80       # Temperature where fan starts ramping
T_MAX=140      # Temperature where fan hits max

HYST=4         # Degrees F required to reduce fan speed
# 3=slow cooldown 10=fast cooldown
MAX_DROP=4     # Maximum PWM decrease per cycle
INTERVAL=1     # Seconds between checks

LAST_PWM=0
LAST_TEMP=0

get_temp() {
    sensors -f 2>/dev/null | awk '/Package id 0/ {gsub(/\+|°F/,"",$4); print $4}'
}

calc_pwm() {
    local T=$1

    if (( $(echo "$T <= $T_MIN" | bc -l) )); then
        echo $MIN_PWM
        return
    fi

    if (( $(echo "$T >= $T_MAX" | bc -l) )); then
        echo $MAX_PWM
        return
    fi

    # Exponential curve (curve steepness)
    local b=0.045
    local pwm=$(echo "$MIN_PWM + ($MAX_PWM - $MIN_PWM) * (e($b * ($T - $T_MIN)) - 1) / (e($b * ($T_MAX - $T_MIN)) - 1)" | bc -l)

    pwm=${pwm%.*}
    if (( pwm < MIN_PWM )); then pwm=$MIN_PWM; fi
    if (( pwm > MAX_PWM )); then pwm=$MAX_PWM; fi

    echo $pwm
}

apply_hysteresis() {
    local T=$1
    local new_pwm=$2

    # Rising temps → no hysteresis
    if (( $(echo "$T > $LAST_TEMP" | bc -l) )); then
        echo $new_pwm
        return
    fi

    # Falling temps → require HYST degrees drop
    if (( $(echo "$LAST_TEMP - $T >= $HYST" | bc -l) )); then
        echo $new_pwm
    else
        echo $LAST_PWM
    fi
}

rate_limit_down() {
    local new=$1
    local old=$2

    # Only limit downward movement
    if (( new < old )); then
        local diff=$(( old - new ))
        if (( diff > MAX_DROP )); then
            echo $(( old - MAX_DROP ))
            return
        fi
    fi

    echo $new
}

# Change the color of the Power LED to
# reflect the CPU package temperature
set_power_led() {
	# CPU >= 176F red
	if (( $(echo "$TEMP > 175" | bc -l) )); then
		echo 3 > /sys/class/leds/power/color
		return
	fi
	# CPU >= 160F orange
	if (( $(echo "$TEMP > 159" | bc -l) )); then
		echo 2 > /sys/class/leds/power/color
		return
	fi
	# CPU >= 144F yellow
	if (( $(echo "$TEMP > 143" | bc -l) )); then
		echo 8 > /sys/class/leds/power/color
		return
	fi
	# CPU >= 128F magenta
	if (( $(echo "$TEMP > 127" | bc -l) )); then
		echo 7 > /sys/class/leds/power/color
		return
	fi
	# CPU >= 112F green
	if (( $(echo "$TEMP > 111" | bc -l) )); then
		echo 4 > /sys/class/leds/power/color
		return
	fi
	# CPU >= 96F cyan
	if (( $(echo "$TEMP > 95" | bc -l) )); then
		echo 6 > /sys/class/leds/power/color
		return
	fi
	# CPU >= 80F blue
	if (( $(echo "$TEMP > 79" | bc -l) )); then
		echo 5 > /sys/class/leds/power/color
		return
	fi
	# CPU < 80F white
	if (( $(echo "$TEMP < 80" | bc -l) )); then
		echo 1 > /sys/class/leds/power/color
		return
	fi
}

while true; do
    TEMP=$(get_temp)
    set_power_led
    RAW_PWM=$(calc_pwm "$TEMP")
    PWM=$(apply_hysteresis "$TEMP" "$RAW_PWM")
    PWM=$(rate_limit_down "$PWM" "$LAST_PWM")

    echo "cpu $PWM" > /proc/it86/fan
    if [ $PWM -lt 145 ]; then
       D_PWM=145
       echo "set $D_PWM" > /proc/it86/fan
    else
       echo "set $PWM" > /proc/it86/fan
    fi

    # Enable for fan speed logging
    #echo "$(date) TEMP=$TEMP PWM=$PWM" >> /var/log/fancontrol.log

    LAST_PWM=$PWM
    LAST_TEMP=$TEMP

    if [ "$PWM" -eq 255 ]; then
        sleep 15
    else
        sleep $INTERVAL
    fi

done

