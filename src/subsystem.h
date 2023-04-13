#pragma once

#include <Arduino.h>
#include "rwlock.h"

/**
 * @brief BaseSubsystem is the base class of all subsystems. It is not to be directly used.
 *
 */
class BaseSubsystem {
 public:
    enum Status {
        INIT,       ///< The initial state
        READY,      ///< After successfully running setup()
        FAULT,      ///< If a fault has occurred
        RUNNING,    ///< Subsystem is running normally
        STOPPED     ///< Subsystem is stopped normally
    };

    /**
     * @brief override this method in subclass to setup the subsystem
     *
     * @return Status
     */
    virtual Status setup() = 0;

    /**
     * @brief override in subclass. Meaningful in only some cases
     *
     * @return Status
     */
    virtual Status start() = 0;

    /**
     * @brief Get the Status of the subsystem
     *
     * @return Status
     */
    Status getStatus() const;

    // to get access to name
    friend class SubsystemManagerClass;

 protected:
    BaseSubsystem();
    virtual ~BaseSubsystem();

    /**
     * @brief Set the Status of the subsystem
     *
     * @param newStatus
     */
    void setStatus(Status newStatus);

    /**
     * @brief Inner status attribute. Do not set/get directly, use setter/getter
     *
     */
    Status status;

    /**
     * @brief The human readable name of the subsystem
     *
     */
    const char *name;

    /**
     * @brief thread safe read/write lock w/ 8 slots
     *
     */
    mutable ReadWriteLock rwLock;
};

/**
 * @brief Inherit from this class if your subsystem needs to be periodically called
 *
 */
class TickableSubsystem : public BaseSubsystem {
 public:
    virtual ~TickableSubsystem();

    /**
     * @brief override this method to implement specific subsystem functionality
     *
     * @return Status
     */
    virtual Status tick() = 0;

    virtual Status start();
};

/**
 * @brief Inherit from this subsystem if your subsystem requires a thread to run
 *
 */
class ThreadedSubsystem : public BaseSubsystem {
 public:
    ThreadedSubsystem();
    virtual ~ThreadedSubsystem();

    /**
     * @brief start the thread
     *
     * @return Status
     */
    Status start();

 protected:
    /**
     * @brief override to return the task priority of your choosing. Defaults to tskIDLE_PRIORITY
     *
     * @return int
     */
    virtual int taskPriority() const;

    /**
     * @brief override to return a parameter to taskFunction(). Defaults to nullptr
     *
     * @return void* const
     */
    virtual void * const taskParameter();

    /**
     * @brief implement to provide a task function for your thread.
     *
     * @note implementations should implement an infinit inner loop
     *
     * @param parameter
     */
    virtual void taskFunction(void *parameter) = 0;

    /**
     * @brief TaskHandle for the thread of this subsystem
     *
     */
    TaskHandle_t taskHandle;


 private:
    static const auto STACK_SIZE = 4096; // 4K ought to be enough for anyone, right?
    StaticTask_t taskBuffer;
    StackType_t taskStack[STACK_SIZE];
};

/**
 * @brief DataThing is designed to provide subsribe read primitives
 *
 * @tparam T
 */
template<class T>
class DataThing {
   public:
      typedef void(DataFn)(const T &, void *args);

      /**
       * @brief Construct a new Data Thing object
       *
       * @note Use this constructor in your subclass's constructor as DataThing<klass>(rwLock)
       *
       * @param locker a ReadWriteLocker to lock
       */
      DataThing(ReadWriteLock &locker) : lock(locker), numCallbacks(0) {}

      virtual ~DataThing() {}

      /**
       * @brief register a callback to be called when Data changes
       *
       * @param fn a function to be called with const reference to data
       * @param args additional arguments to be call function with
       */
      void registerCallback(DataThing<T>::DataFn fn, void *args) {
         lock.Lock();
         if (numCallbacks == MAX_CALLBACKS) {
            //Log.errorln("Tried to add beyond %d callbacks", MAX_CALLBACKS);
            lock.UnLock();
            return;
         }
         callback cb {
            .args = args,
            .fn = fn
         };
         callbacks[numCallbacks] = cb;
         numCallbacks++;
         lock.UnLock();
      }

      /**
       * @brief read underlying data
       *
       * @param fn a function to be called with const reference to data
       * @param args additional arguments to be call function with
       */
      void readData(DataThing<T>::DataFn fn, void *args) const {
         lock.RLock();
         fn(data, args);
         lock.RUnlock();
      }

   protected:
      /**
       * @brief call this method to call callbacks registered with registerCallback()
       *
       */
      virtual void callCallbacks() {
         lock.RLock();
         auto n = numCallbacks;
         lock.RUnlock();

         for (auto i = 0; i < n; i++) {
            lock.RLock();
            callback cb = callbacks[i];
            cb.fn(data, cb.args);
            lock.RUnlock();
         }
      }

      /**
       * @brief The actual data itself
       *
       */
      T data;

   private:
      static constexpr size_t MAX_CALLBACKS = 8;

      DataThing() = delete;
      DataThing(const DataThing& other) = delete;

      ReadWriteLock &lock;
      int numCallbacks;

      struct callback {
         void *args;
         void (*fn)(const T&, void*);
      };
      callback callbacks[MAX_CALLBACKS];
};

/**
 * @brief SubsystemManager allows for consistent starting of subsystems without cluttering main
 *
 */
class SubsystemManagerClass : public BaseSubsystem {
public:
   /**
    * @brief Specification for dependencies of a threaded subsystem
    *
    */
   struct Spec {
      Spec(BaseSubsystem *subsys, BaseSubsystem** deps);
      /**
       * @brief a pointer to the subsystem to add
       *
       */
      BaseSubsystem *subsystem;

      /**
       * @brief null terminated array of threaded subsystem dependencies of this one
       *
       */
      BaseSubsystem **deps;
      Spec *next;
   };

   SubsystemManagerClass();
   virtual ~SubsystemManagerClass();

   /**
    * @brief add a subsystem to the subsystem manager
    *
    * @param spec the dependency specification of this subsystem
    *
    * In your constructor, use as:
    * static BaseSubsystem* deps[] = {&DependentSubsystemIstance, NULL};
    * static SubsystemManagerClass::Spec spec = {
    *    .subsystem = this,
    *    .deps = deps,
    * };
    * SubsystemManager.addSubsystem(&spec);
    */
   void addSubsystem(Spec *spec);
   Status setup();
   Status start();

private:
   Spec* specs;

   Spec* findSpecBySubsystem(BaseSubsystem *needle);
   void descendAndStartOrSetup(Spec *spec, BaseSubsystem::Status desiredState);
};

extern SubsystemManagerClass SubsystemManager;

