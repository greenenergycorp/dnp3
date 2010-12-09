//
// Licensed to Green Energy Corp (www.greenenergycorp.com) under one
// or more contributor license agreements. See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  Green Energy Corp licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.
//
#ifndef __ASYNC_PHYS_LAYER_MONITOR_H_
#define __ASYNC_PHYS_LAYER_MONITOR_H_


#include "IHandlerAsync.h"
#include "TimerInterfaces.h"
#include "IPhysMonitor.h"
#include "Logger.h"

namespace apl {

	class IPhysicalLayerAsync;

	/// Class keeps a physical layer open by kicking off
	/// AsyncOpen requests on a timer
	class AsyncPhysLayerMonitor : public IHandlerAsync
	{
		public:
			AsyncPhysLayerMonitor(Logger*, IPhysicalLayerAsync*, ITimerSource*, millis_t aOpenRetry);
			~AsyncPhysLayerMonitor();

			void Start();
			void Stop();

			bool IsRunning();
			bool IsOpen();

			void SetMonitor(IPhysMonitor* apMonitor);

		protected:

			bool Opening() const { return mOpening; }

			IPhysicalLayerAsync* mpPhys;
			ITimerSource* mpTimerSrc;
			ITimer* mpOpenTimer;
			LogVariable mPortState;

			virtual void Up() = 0;
			virtual void Down() = 0;

		private:

			void Notify(IPhysMonitor::State);

			bool mOpening;
			bool mOpen;
			bool mStopOpenRetry;
			const millis_t M_OPEN_RETRY;

			/// Implement from IHandlerAsync - Try to reconnect using a timer
			void _OnOpenFailure();
			void _OnLowerLayerUp();
			void _OnLowerLayerDown();

			IPhysMonitor* mpMonitor;

	};
}

#endif
