/*
 * CAN bus IO logic.
 * Copyright (C) 2014 Pavel Kirienko <pavel.kirienko@gmail.com>
 */

#pragma once

#include <cassert>
#include <stdint.h>
#include <uavcan/linked_list.hpp>
#include <uavcan/dynamic_memory.hpp>
#include <uavcan/impl_constants.hpp>
#include <uavcan/util/compile_time.hpp>
#include <uavcan/can_driver.hpp>
#include <uavcan/system_clock.hpp>
#include <uavcan/time.hpp>

namespace uavcan
{

struct CanRxFrame : public CanFrame
{
    MonotonicTime ts_mono;
    UtcTime ts_utc;
    uint8_t iface_index;

    CanRxFrame()
    : iface_index(0)
    { }

    std::string toString(StringRepresentation mode = StrTight) const;
};


class CanTxQueue : Noncopyable
{
public:
    enum Qos { Volatile, Persistent };

    struct Entry : public LinkedListNode<Entry>  // Not required to be packed - fits the block in any case
    {
        MonotonicTime deadline;
        CanFrame frame;
        uint8_t qos;

        Entry(const CanFrame& frame, MonotonicTime deadline, Qos qos)
        : deadline(deadline)
        , frame(frame)
        , qos(uint8_t(qos))
        {
            assert(qos == Volatile || qos == Persistent);
            IsDynamicallyAllocatable<Entry>::check();
        }

        static void destroy(Entry*& obj, IAllocator& allocator);

        bool isExpired(MonotonicTime timestamp) const { return timestamp > deadline; }

        bool qosHigherThan(const CanFrame& rhs_frame, Qos rhs_qos) const;
        bool qosLowerThan(const CanFrame& rhs_frame, Qos rhs_qos) const;
        bool qosHigherThan(const Entry& rhs) const { return qosHigherThan(rhs.frame, Qos(rhs.qos)); }
        bool qosLowerThan(const Entry& rhs)  const { return qosLowerThan(rhs.frame, Qos(rhs.qos)); }

        std::string toString() const;
    };

private:
    class PriorityInsertionComparator
    {
        const CanFrame& frm_;
    public:
        PriorityInsertionComparator(const CanFrame& frm) : frm_(frm) { }
        bool operator()(const Entry* entry)
        {
            assert(entry);
            return frm_.priorityHigherThan(entry->frame);
        }
    };

    LinkedListRoot<Entry> queue_;
    IAllocator* const allocator_;
    ISystemClock* const sysclock_;
    uint32_t rejected_frames_cnt_;

    void registerRejectedFrame();

public:
    CanTxQueue()
    : allocator_(NULL)
    , sysclock_(NULL)
    , rejected_frames_cnt_(0)
    { }

    CanTxQueue(IAllocator* allocator, ISystemClock* sysclock)
    : allocator_(allocator)
    , sysclock_(sysclock)
    , rejected_frames_cnt_(0)
    { }

    ~CanTxQueue();

    void push(const CanFrame& frame, MonotonicTime tx_deadline, Qos qos);

    Entry* peek();               // Modifier
    void remove(Entry*& entry);

    bool topPriorityHigherOrEqual(const CanFrame& rhs_frame) const;

    uint32_t getNumRejectedFrames() const { return rejected_frames_cnt_; }

    bool isEmpty() const { return queue_.isEmpty(); }
};


class CanIOManager : Noncopyable
{
public:
    enum { MaxIfaces = 3 };

private:
    ICanDriver& driver_;
    ISystemClock& sysclock_;

    CanTxQueue tx_queues_[MaxIfaces];

    // Noncopyable
    CanIOManager(CanIOManager&);
    CanIOManager& operator=(CanIOManager&);

    int sendToIface(int iface_index, const CanFrame& frame, MonotonicTime tx_deadline);
    int sendFromTxQueue(int iface_index);
    int makePendingTxMask() const;

public:
    CanIOManager(ICanDriver& driver, IAllocator& allocator, ISystemClock& sysclock)
    : driver_(driver)
    , sysclock_(sysclock)
    {
        assert(driver.getNumIfaces() <= MaxIfaces);
        // We can't initialize member array with non-default constructors in C++03
        for (int i = 0; i < MaxIfaces; i++)
        {
            tx_queues_[i].~CanTxQueue();
            new (tx_queues_ + i) CanTxQueue(&allocator, &sysclock);
        }
    }

    int getNumIfaces() const;

    uint64_t getNumErrors(int iface_index) const;

    /**
     * Returns:
     *  0 - rejected/timedout/enqueued
     *  1+ - sent/received
     *  negative - failure
     */
    int send(const CanFrame& frame, MonotonicTime tx_deadline, MonotonicTime blocking_deadline,
             int iface_mask, CanTxQueue::Qos qos);
    int receive(CanRxFrame& frame, MonotonicTime blocking_deadline);
};

}