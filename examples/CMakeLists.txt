add_executable (sftp sftp.cpp)
target_link_libraries (sftp axon ${LIBRARIES})
#target_link_libraries (blah tcn avro rdkafka serdes)
#
add_executable (license license.cpp)
target_link_libraries (license axon ${LIBRARIES})
#
add_executable (ftp sftp.cpp)
target_link_libraries (ftp axon ${LIBRARIES})
#
add_executable (cfg cfg.cpp)
target_link_libraries (cfg axon ${LIBRARIES})
#
#add_executable (cfg2 cfg2.cpp)
#target_link_libraries (cfg2 axon)
#
add_executable (read_topic read_topic.cpp)
target_link_libraries (read_topic axon ${LIBRARIES})

add_executable (smb smb.cpp)
target_link_libraries (smb axon ${LIBRARIES})

add_executable (krb5 krb5.cpp)
target_link_libraries (krb5 axon ${LIBRARIES})

add_executable (connector connector.cpp)
target_link_libraries (connector axon ${LIBRARIES})

add_executable (mail mail.cpp)
target_link_libraries (mail axon ${LIBRARIES})

add_executable (logger logger.cpp)
target_link_libraries (logger axon ${LIBRARIES})
