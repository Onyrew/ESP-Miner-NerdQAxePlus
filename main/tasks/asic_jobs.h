#pragma once

#include <memory>
#include <new>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include "macros.h"
#include "mining.h"

#define MAX_ASIC_JOBS 128

class AsicJobs {
    struct SnapshotDeleter {
        void operator()(bm_job *job) const { free(job); }
    };

    bm_job *m_activeJobs[MAX_ASIC_JOBS];
    pthread_mutex_t m_validJobsLock;

public:
    using JobSnapshot = std::unique_ptr<bm_job, SnapshotDeleter>;

    AsicJobs() {
        m_validJobsLock = PTHREAD_MUTEX_INITIALIZER;
        memset(m_activeJobs, 0, sizeof(m_activeJobs));
    }

    int cleanJobs(int pool) {
        PThreadGuard g(m_validJobsLock);
        int deleted = 0;
        for (int i = 0; i < MAX_ASIC_JOBS; i++) {
            if (m_activeJobs[i] && m_activeJobs[i]->pool_id == pool) {
                free_bm_job(m_activeJobs[i]);
                m_activeJobs[i] = 0;
                deleted++;
            }
        }
        return deleted;
    }

    void storeJob(bm_job *next_job, uint8_t asic_job_id) {
        PThreadGuard g(m_validJobsLock);
        // if a slot was used before free it
        if (m_activeJobs[asic_job_id]) {
            free_bm_job(m_activeJobs[asic_job_id]);
        }
        // save job into slot
        m_activeJobs[asic_job_id] = next_job;
    }

    JobSnapshot getSnapshot(uint8_t asic_job_id) {
        if (asic_job_id >= MAX_ASIC_JOBS) {
            return {};
        }

        PThreadGuard g(m_validJobsLock);
        const bm_job *job = m_activeJobs[asic_job_id];
        if (!job || !job->jobid || !job->extranonce2) {
            return {};
        }

        // Store both strings after the job so the snapshot needs one allocation.
        const size_t jobIdSize = strlen(job->jobid) + 1;
        const size_t extranonceSize = strlen(job->extranonce2) + 1;
        auto *storage = static_cast<char *>(MALLOC(sizeof(bm_job) + jobIdSize + extranonceSize));
        if (!storage) {
            return {};
        }
        auto *snapshot = new (storage) bm_job(*job);
        snapshot->jobid = storage + sizeof(bm_job);
        snapshot->extranonce2 = snapshot->jobid + jobIdSize;
        memcpy(snapshot->jobid, job->jobid, jobIdSize);
        memcpy(snapshot->extranonce2, job->extranonce2, extranonceSize);
        return JobSnapshot(snapshot);
    }

};


