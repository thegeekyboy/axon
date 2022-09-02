[Unit]
Description=Sentinel Mediation Services
After=network.target
ConditionPathExists=/home/hyperion/config/hyperion.cfg
#StartLimitIntervalSec=0

[Service]
Type=forking
PIDFile=/home/hyperion/logs/hyperion.pid
#Restart=always
RestartSec=5
User=hyperion
EnvironmentFile=/home/hyperion/env
ExecStart=@CMAKE_INSTALL_PREFIX@/bin/hyperion -c /home/hyperion/config/hyperion.cfg

[Install]
WantedBy=multi-user.target
