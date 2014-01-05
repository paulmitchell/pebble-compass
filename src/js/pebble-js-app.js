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

var oldHeading = -1;

function locationSuccess(position) {
  var newHeading = Math.round(position.coords.heading);
  if (newHeading != oldHeading) {
    console.log('Location received: ' + newHeading);
    Pebble.sendAppMessage({ 'heading': newHeading });
    oldHeading = newHeading;
  }
}

function locationError(err) {
  Pebble.showSimpleNotificationOnPebble('Compass', 'Error (' + err.code + '): ' + err.message);
}

var locationOptions = {
  'enableHighAccuracy': true
}; 

Pebble.addEventListener('ready',
  function(e) {
    locationWatcher = window.navigator.geolocation.watchPosition(locationSuccess, locationError, locationOptions);
  });
