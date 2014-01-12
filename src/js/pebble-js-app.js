/*
 * Copyright 2014 Paul Mitchell
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

var STATUS_SUCCESS = 0
var STATUS_NOT_MOVING = -1
var STATUS_WAITING = -2
var STATUS_PERMISSION_DENIED = -3
var STATUS_POSITION_UNAVAILABLE = -4
var STATUS_TIMEOUT = -5

var oldHeading = Number.NaN;

function locationSuccess(position) {
  // console.log('Location success: ' + JSON.stringify(position));
  var newHeading = Math.round(position.coords.heading);
  if (newHeading != oldHeading) {
    console.log('Heading updated: ' + newHeading);
    Pebble.sendAppMessage({ 'heading': newHeading });
    oldHeading = newHeading;
  }
}

function locationError(err) {
  console.log('Location error (' + err.code + '): ' + err.message);
  var status = STATUS_SUCCESS;
  switch (err.code) {
  case err.PERMISSION_DENIED:
    status = STATUS_PERMISSION_DENIED;
    break;
  case err.POSITION_UNAVAILABLE:
    status = STATUS_POSITION_UNAVAILABLE;
    break;
  case err.TIMEOUT:
    status = STATUS_TIMEOUT;
    break;
  }
  Pebble.sendAppMessage({ 'heading': status });
  oldHeading = Number.NaN;
}

var locationOptions = {
  'enableHighAccuracy': true,
  'timeout': 15000
};

Pebble.addEventListener('ready',
  function(e) {
    console.log('Compass ready');
    locationWatcher = window.navigator.geolocation.watchPosition(locationSuccess, locationError, locationOptions);
  });
