/*
 * Copyright Amazon.com, Inc. and its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License. See the LICENSE accompanying this file
 * for the specific language governing permissions and limitations under
 * the License.
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "unity.h"

#include "MQTTFileDownloader.h"

/* ============================   TEST GLOBALS ============================= */

char * thingName = "thingname";
size_t thingNameLength = strlen("thingname");
char * streamName = "stream-name";
size_t streamNameLength = strlen("stream-name");

uint8_t initResult;
/* ===========================   UNITY FIXTURES ============================ */

/* Called before each test method. */
void setUp()
{
    initResult = 10; // An unimplimented MQTTFileDownloaderStatus value
}

/* Called after each test method. */
void tearDown()
{
}

/* Called at the beginning of the whole suite. */
void suiteSetUp()
{
}

/* Called at the end of the whole suite. */
int suiteTearDown( int numFailures )
{
    return numFailures;
}

/* ===============================   TESTS   =============================== */

void test_init_returnsBadParam_givenNullContext( void )
{
    initResult = mqttDownloader_init(NULL, streamName, streamNameLength, thingName, thingNameLength, DATA_TYPE_JSON);

    TEST_ASSERT_EQUAL(MQTTFileDownloaderBadParameter, initResult);
}