/* -*- C++ -*-; c-basic-offset: 4; indent-tabs-mode: nil */
/*
 * Include file for KeyedRateLimiter
 *
 * Copyright (c) 2015 Cisco Systems, Inc. and others.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

#pragma once
#ifndef OVSAGENT_KEYED_RATE_LIMITER_H
#define OVSAGENT_KEYED_RATE_LIMITER_H

#include <boost/noncopyable.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_guard.hpp>
#include <boost/foreach.hpp>
#include <boost/chrono/duration.hpp>
#include <boost/chrono/system_clocks.hpp>
#include <boost/unordered_set.hpp>

#include <string>

namespace ovsagent {

/**
 * A class that helps to enforce that operations related to some
 * specific key can be performed only at some fixed rate.  The
 * BUCKET_TIME indicates how precise the timer will be.  The maximum
 * rate will be one in ((NBUCKETS-1) * BUCKET_TIME) time units, but
 * could be as little as one in (NBUCKETS * BUCKET_TIME) time units.
 *
 * @param K the key type; must be hashable
 * @param NBUCKETS the number of buckets for the rate limiter
 * @param BUCKET_TIME the amount of time to allow for each "tick" on
 * the buckets in milliseconds
 */
template <typename K,
          const size_t NBUCKETS,
          const uint64_t BUCKET_TIME>
class KeyedRateLimiter : private boost::noncopyable {
public:
    /**
     * Instantiate a keyed rate limiter with the specified number of
     * buckets and bucket duration.
     */
    KeyedRateLimiter()
        : buckets(NBUCKETS), curBucket(0),
          curBucketStart(boost::chrono::steady_clock::now()) { }

    /**
     * Clear the rate limiter and reset its state to the initial state
     */
    void clear() {
        boost::lock_guard<boost::mutex> guard(mtx);
        do_clear();
    }

    /**
     * Apply the rate limiter to the given key.  Returns true if the
     * key has not occurred too recently
     *
     * @param key the key to check
     * @return true to indicate the event should be handled, otherwise
     * false.
     */
    bool event(K key) {
        time_point now = boost::chrono::steady_clock::now();

        boost::lock_guard<boost::mutex> guard(mtx);

        duration increment(BUCKET_TIME);
        if (curBucketStart + increment * NBUCKETS < now) {
            // special case for very slow rate
            do_clear();
            buckets[curBucket].insert(key);
            return true;
        } else {
            // advance the timer
            while (curBucketStart + increment < now) {
                curBucketStart += increment;
                curBucket = (curBucket+1) % NBUCKETS;
                buckets[curBucket].clear();
            }
        }

        // check if any buckets in the window have the key
        BOOST_FOREACH(key_set& ks, buckets) {
            if (ks.find(key) != ks.end())
                return false;
        }

        buckets[curBucket].insert(key);
        return true;
    }

private:
    typedef boost::chrono::steady_clock::time_point time_point;
    typedef boost::chrono::milliseconds duration;
    typedef boost::unordered_set<K> key_set;

    boost::mutex mtx;
    std::vector<key_set> buckets;
    size_t curBucket;
    time_point curBucketStart;

    void do_clear() {
        BOOST_FOREACH(key_set& ks, buckets)
            ks.clear();
        curBucket = 0;
        curBucketStart = boost::chrono::steady_clock::now();
    }
};

} /* namespace ovsagent */

#endif /* OVSAGENT_KEYED_RATE_LIMITER */
