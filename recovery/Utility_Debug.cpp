//
// Created by Frank Steiler on 9/28/16.
// Copyright (c) 2016 Hewlett-Packard. All rights reserved.
//
// Utility_Debug.cpp: [...]
//

#include "Utility.h"

QMap<QString, QVariant>* Utility::Debug::getRaspbianJSON() {
    QMap<QString, QVariant> *raspbian = new QMap<QString, QVariant>();
    raspbian->insert("description", "A community-created port of Debian jessie for the Raspberry Pi");
    raspbian->insert("feature_level", 35120124);
    raspbian->insert("icon", "http://downloads.raspberrypi.org/raspbian/Raspbian.png");
    raspbian->insert("marketing_info", "http://downloads.raspberrypi.org/raspbian/marketing.tar");
    raspbian->insert("nominal_size", 3615);
    raspbian->insert("os_info", "http://downloads.raspberrypi.org/raspbian/os.json");
    raspbian->insert("os_name", "Raspbian");
    raspbian->insert("partition_setup", "http://downloads.raspberrypi.org/raspbian/partition_setup.sh");
    raspbian->insert("partition_info", "http://downloads.raspberrypi.org/raspbian/partitions.json");
    raspbian->insert("release_date", "2016-05-27");
    raspbian->insert("supported_hex_revisions", "2,3,4,5,6,7,8,9,d,e,f,10,11,12,14,19,1040,1041,0092,0093,2082");
    QVariantList *supported_models = new QVariantList();
    supported_models->append("Pi Model");
    supported_models->append("Pi 2");
    supported_models->append("Pi Zero");
    supported_models->append("Pi 3");
    raspbian->insert("supported_models", *supported_models);
    QVariantList *tarballs = new QVariantList();
    tarballs->append("http://downloads.raspberrypi.org/raspbian/boot.tar.xz");
    tarballs->append("http://downloads.raspberrypi.org/raspbian/root.tar.xz");
    raspbian->insert("tarballs", *tarballs);

    return raspbian;
}