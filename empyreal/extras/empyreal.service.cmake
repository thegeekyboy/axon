[Unit]
Description=Sentinel Mediation Services
After=network.target
ConditionPathExists=/home/sentinel/config/sentinel.cfg
#StartLimitIntervalSec=0

[Service]
Type=forking
PIDFile=/home/sentinel/logs/sentinel.pid
#Restart=always
RestartSec=5
User=sentinel
EnvironmentFile=/home/sentinel/env
ExecStart=@CMAKE_INSTALL_PREFIX@/bin/sentinel -c /home/sentinel/config/sentinel.cfg

[Install]
WantedBy=multi-user.target
