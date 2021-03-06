if (BUILD_TESTING)
  set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_LIST_DIR}/bin)

  add_executable(
    DirectoryEntryTest
    EntryTest.cpp
    ${QSFS_SOURCE_DIR}/data/Entry.cpp
    $<TARGET_OBJECTS:qsfsFileMetaData>
    ${QSFS_SOURCE_DIR}/data/DirectoryTree.cpp
    ${QSFS_SOURCE_DIR}/data/Node.cpp
    ${QSFS_SOURCE_DIR}/base/TimeUtils.cpp
    $<TARGET_OBJECTS:qsfsLogging>
  )
  if (APPLE)
    target_link_libraries(DirectoryEntryTest osxfuse osxboost_thread)
  elseif (UNIX)
    target_link_libraries(DirectoryEntryTest fuse boost_thread)
  endif ()
  target_link_libraries(DirectoryEntryTest gtest glog gflags ${CMAKE_THREAD_LIBS_INIT})
  add_test(NAME qsfs_directory_entry COMMAND DirectoryEntryTest)

  add_executable(
    DirectoryNodeTest
    NodeTest.cpp
    ${QSFS_SOURCE_DIR}/data/Entry.cpp
    ${QSFS_SOURCE_DIR}/data/Node.cpp
    $<TARGET_OBJECTS:qsfsFileMetaData>
    ${QSFS_SOURCE_DIR}/data/DirectoryTree.cpp
    ${QSFS_SOURCE_DIR}/base/TimeUtils.cpp
    $<TARGET_OBJECTS:qsfsLogging>
  )
  if (APPLE)
    target_link_libraries(DirectoryNodeTest osxfuse osxboost_thread)
  elseif (UNIX)
    target_link_libraries(DirectoryNodeTest fuse boost_thread)
  endif () 
  target_link_libraries(DirectoryNodeTest gtest glog gflags ${CMAKE_THREAD_LIBS_INIT})
  add_test(NAME qsfs_directory_node COMMAND DirectoryNodeTest)

  add_executable(
    DirectoryTreeTest
    DirectoryTreeTest.cpp
    ${QSFS_SOURCE_DIR}/data/DirectoryTree.cpp
    ${QSFS_SOURCE_DIR}/data/Entry.cpp
    ${QSFS_SOURCE_DIR}/data/Node.cpp
    ${QSFS_SOURCE_DIR}/base/TimeUtils.cpp
    $<TARGET_OBJECTS:qsfsFileMetaData>
    $<TARGET_OBJECTS:qsfsLogging>
  )
  if (APPLE)
    target_link_libraries(DirectoryTreeTest osxfuse osxboost_thread)
  elseif (UNIX)
    target_link_libraries(DirectoryTreeTest fuse boost_thread)
  endif ()
  target_link_libraries(DirectoryTreeTest gtest glog gflags ${CMAKE_THREAD_LIBS_INIT})
  add_test(NAME qsfs_directory_tree COMMAND DirectoryTreeTest)

  add_executable(
    ExceptionTest
    ExceptionTest.cpp
  )
  target_link_libraries(ExceptionTest gtest ${CMAKE_THREAD_LIBS_INIT})
  add_test(NAME qsfs_exception COMMAND ExceptionTest)

  add_executable(
    FileMetaDataManagerTest
    FileMetaDataManagerTest.cpp
    ${QSFS_SOURCE_DIR}/data/DirectoryTree.cpp
    ${QSFS_SOURCE_DIR}/data/Node.cpp
    ${QSFS_SOURCE_DIR}/data/Entry.cpp
    ${QSFS_SOURCE_DIR}/base/TimeUtils.cpp
    $<TARGET_OBJECTS:qsfsFileMetaData>
    $<TARGET_OBJECTS:qsfsLogging>
  )
  if (APPLE)
    target_link_libraries(FileMetaDataManagerTest osxfuse osxboost_thread)
  elseif (UNIX)
    target_link_libraries(FileMetaDataManagerTest fuse boost_thread)
  endif ()
  target_link_libraries(FileMetaDataManagerTest gtest glog gflags ${CMAKE_THREAD_LIBS_INIT})
  add_test(NAME qsfs_file_metadata_manager COMMAND FileMetaDataManagerTest)

  add_executable(
    HashUtilsTest
    HashUtilsTest.cpp
  )
  target_link_libraries(HashUtilsTest gtest ${CMAKE_THREAD_LIBS_INIT})
  add_test(NAME qsfs_hashutils COMMAND HashUtilsTest)

  add_executable(
    LogLevelTest
    LogLevelTest.cpp
    ${QSFS_SOURCE_DIR}/base/StringUtils.cpp
    ${QSFS_SOURCE_DIR}/base/LogLevel.cpp
  )
  target_link_libraries(LogLevelTest gtest ${CMAKE_THREAD_LIBS_INIT})
  add_test(NAME qsfs_loglevel COMMAND LogLevelTest)

  add_executable(
    LoggingTest
    LoggingTest.cpp
    $<TARGET_OBJECTS:qsfsLogging>
  )
  if (APPLE)
    target_link_libraries(LoggingTest osxfuse osxboost_thread)
  elseif (UNIX)
    target_link_libraries(LoggingTest fuse boost_thread)
  endif ()
  target_link_libraries(LoggingTest gtest glog gflags ${CMAKE_THREAD_LIBS_INIT})
  add_test(NAME qsfs_logging COMMAND LoggingTest)

  add_executable(
    PageTest
    PageTest.cpp
   ${QSFS_SOURCE_DIR}/data/Page.cpp
   ${QSFS_SOURCE_DIR}/base/UtilsWithLog.cpp
    $<TARGET_OBJECTS:qsfsStream>
    $<TARGET_OBJECTS:qsfsLogging>
  )
  if (APPLE)
    target_link_libraries(PageTest osxfuse osxboost_thread)
  elseif (UNIX)
    target_link_libraries(PageTest fuse boost_thread)
  endif ()
  target_link_libraries(PageTest gtest glog gflags ${CMAKE_THREAD_LIBS_INIT})
  add_test(NAME qsfs_page COMMAND PageTest)

  add_executable(
    FileTest
    FileTest.cpp
    ${QSFS_SOURCE_DIR}/client/ClientConfiguration.cpp
    ${QSFS_SOURCE_DIR}/client/Credentials.cpp
    ${QSFS_SOURCE_DIR}/client/Protocol.cpp
    ${QSFS_SOURCE_DIR}/client/TransferHandle.cpp
    ${QSFS_SOURCE_DIR}/client/TransferManager.cpp
    ${QSFS_SOURCE_DIR}/client/QSTransferManager.cpp
    ${QSFS_SOURCE_DIR}/client/NullClient.cpp
    ${QSFS_SOURCE_DIR}/client/Client.cpp
    ${QSFS_SOURCE_DIR}/client/QSError.cpp
    ${QSFS_SOURCE_DIR}/client/Utils.cpp
    ${QSFS_SOURCE_DIR}/client/RetryStrategy.cpp
    ${QSFS_SOURCE_DIR}/data/Page.cpp
    ${QSFS_SOURCE_DIR}/data/File.cpp
    ${QSFS_SOURCE_DIR}/data/Cache.cpp
    ${QSFS_SOURCE_DIR}/data/DirectoryTree.cpp
    ${QSFS_SOURCE_DIR}/data/Entry.cpp
    ${QSFS_SOURCE_DIR}/data/Node.cpp
    ${QSFS_SOURCE_DIR}/data/FileMetaDataManager.cpp
    ${QSFS_SOURCE_DIR}/data/FileMetaData.cpp
    ${QSFS_SOURCE_DIR}/data/ResourceManager.cpp
    ${QSFS_SOURCE_DIR}/base/UtilsWithLog.cpp
    ${QSFS_SOURCE_DIR}/base/TimeUtils.cpp
    ${QSFS_SOURCE_DIR}/base/ThreadPool.cpp
    ${QSFS_SOURCE_DIR}/base/TaskHandle.cpp
    ${QSFS_SOURCE_DIR}/base/ThreadPoolInitializer.cpp
    $<TARGET_OBJECTS:qsfsStream>
    $<TARGET_OBJECTS:qsfsLogging>
  )
  if (APPLE)
    target_link_libraries(FileTest osxfuse osxboost_thread)
  elseif (UNIX)
    target_link_libraries(FileTest fuse boost_thread)
  endif ()
  target_link_libraries(FileTest gtest glog gflags ${CMAKE_THREAD_LIBS_INIT})
  add_test(NAME qsfs_file COMMAND FileTest)

  add_executable(
    CacheTest
    CacheTest.cpp
    ${QSFS_SOURCE_DIR}/client/ClientConfiguration.cpp
    ${QSFS_SOURCE_DIR}/client/Credentials.cpp
    ${QSFS_SOURCE_DIR}/client/Protocol.cpp
    ${QSFS_SOURCE_DIR}/client/TransferHandle.cpp
    ${QSFS_SOURCE_DIR}/client/TransferManager.cpp
    ${QSFS_SOURCE_DIR}/client/QSTransferManager.cpp
    ${QSFS_SOURCE_DIR}/client/NullClient.cpp
    ${QSFS_SOURCE_DIR}/client/Client.cpp
    ${QSFS_SOURCE_DIR}/client/QSError.cpp
    ${QSFS_SOURCE_DIR}/client/Utils.cpp
    ${QSFS_SOURCE_DIR}/client/RetryStrategy.cpp
    ${QSFS_SOURCE_DIR}/data/Page.cpp
    ${QSFS_SOURCE_DIR}/data/File.cpp
    ${QSFS_SOURCE_DIR}/data/Cache.cpp
    ${QSFS_SOURCE_DIR}/data/DirectoryTree.cpp
    ${QSFS_SOURCE_DIR}/data/Entry.cpp
    ${QSFS_SOURCE_DIR}/data/Node.cpp
    ${QSFS_SOURCE_DIR}/data/ResourceManager.cpp
    ${QSFS_SOURCE_DIR}/base/UtilsWithLog.cpp
    ${QSFS_SOURCE_DIR}/base/TimeUtils.cpp
    ${QSFS_SOURCE_DIR}/base/ThreadPool.cpp
    ${QSFS_SOURCE_DIR}/base/TaskHandle.cpp
    ${QSFS_SOURCE_DIR}/base/ThreadPoolInitializer.cpp
    $<TARGET_OBJECTS:qsfsStream>
    $<TARGET_OBJECTS:qsfsFileMetaData>
    $<TARGET_OBJECTS:qsfsLogging>
  )
  if (APPLE)
    target_link_libraries(CacheTest osxfuse osxboost_thread)
  elseif (UNIX)
    target_link_libraries(CacheTest fuse boost_thread)
  endif ()
  target_link_libraries(CacheTest gtest glog gflags ${CMAKE_THREAD_LIBS_INIT})
  add_test(NAME qsfs_cache COMMAND CacheTest)

  add_executable(
    ResourceManagerTest
    ResourceManagerTest.cpp
    ${QSFS_SOURCE_DIR}/data/ResourceManager.cpp
    $<TARGET_OBJECTS:qsfsLogging>
  )
  if (APPLE)
    target_link_libraries(ResourceManagerTest osxfuse osxboost_thread)
  elseif (UNIX)
    target_link_libraries(ResourceManagerTest fuse boost_thread)
  endif ()
  target_link_libraries(ResourceManagerTest gtest glog gflags ${CMAKE_THREAD_LIBS_INIT})
  add_test(NAME qsfs_resource_manager COMMAND ResourceManagerTest)

  add_executable(
    StreamTest
    StreamTest.cpp
    $<TARGET_OBJECTS:qsfsStream>
    $<TARGET_OBJECTS:qsfsLogging>
  )
  if (APPLE)
    target_link_libraries(StreamTest osxfuse osxboost_thread)
  elseif (UNIX)
    target_link_libraries(StreamTest fuse boost_thread)
  endif ()
  target_link_libraries(StreamTest gtest glog gflags ${CMAKE_THREAD_LIBS_INIT})
  add_test(NAME qsfs_stream COMMAND StreamTest)

  add_executable(
    StringUtilsTest
    StringUtilsTest.cpp
    ${QSFS_SOURCE_DIR}/base/StringUtils.cpp
  )
  target_link_libraries(StringUtilsTest gtest ${CMAKE_THREAD_LIBS_INIT})
  add_test(NAME qsfs_stringutils COMMAND StringUtilsTest)

  add_executable(
    ThreadPoolTest
    ThreadPoolTest.cpp
    ${QSFS_SOURCE_DIR}/base/ThreadPool.cpp
    ${QSFS_SOURCE_DIR}/base/TaskHandle.cpp
  )
  if (APPLE)
    target_link_libraries(ThreadPoolTest osxboost_thread)
  elseif (UNIX)
    target_link_libraries(ThreadPoolTest boost_thread)
  endif ()
  target_link_libraries(ThreadPoolTest gtest ${CMAKE_THREAD_LIBS_INIT})
  add_test(NAME qsfs_threadpool COMMAND ThreadPoolTest)

  add_executable(
    TimeUtilsTest
    TimeUtilsTest.cpp
    ${QSFS_SOURCE_DIR}/base/TimeUtils.cpp
  )
  target_link_libraries(TimeUtilsTest gtest ${CMAKE_THREAD_LIBS_INIT})
  add_test(NAME qsfs_timeutils COMMAND TimeUtilsTest)

endif (BUILD_TESTING)