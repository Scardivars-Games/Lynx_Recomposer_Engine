# Scene loading regression checks

From the repository root (C++23, assertions enabled):

```sh
c++ -std=c++23 -pthread -I include tests/command_buffer_concurrency.cpp -o /tmp/ilen-command-buffer-test
/tmp/ilen-command-buffer-test

c++ -std=c++23 -pthread -DILEN_DISABLE_BASIC_LOGGIN=1 -I include -I project_data tests/scene_loading.cpp src/Common/resources/scene_manager.cpp src/Common/ecs/ecs.cpp src/Common/core/thread-context.cpp -o /tmp/ilen-scene-loading-test
/tmp/ilen-scene-loading-test .ilen-bin-data
```

The queue test checks concurrent producers/consumer, exact-once execution, and
callbacks enqueueing callbacks. The integration test uses the current compiled
`TemporalGroundTest` fixture (92 Grounds plus its spawner), generated serializers,
real ECS and job system. Window/camera systems, logging, resource paths and asset
readiness are stubbed; no GPU or scripts are run. It repeats loading 30 times,
alternating main-thread and worker requests, and verifies all Ground names and
components **before** allowing scene activation. Concurrent scene-load requests
are rejected explicitly instead of replacing an operation in progress.

Loading semantics: the worker reads the scene into owned data; the main thread
instantiates it after the future becomes ready. `AsyncOperation::isDone()` also
includes `Failed`; check `phase` to distinguish success. Synchronous `LoadScene`
requires the main thread. `LoadPrefab` invoked from workers queues instantiation
and returns an empty vector; callers needing immediate entity IDs must call it
on the main thread. The public `EntityAPI::LoadPrefab` remains a void operation.
