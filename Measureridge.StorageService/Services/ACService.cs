using MeasureBridge.Model.Report;
using MeasureBridge.Model.Storage;
using MeasureBridge.StorageService.Interfaces;
using Measureridge.StorageService.Interfaces;
using System;
using System.Linq;
using System.Threading.Tasks;

namespace Measureridge.StorageService.Services
{
    public class ACService : IACService
    {
        protected ITableService<ACCurrent> _service;
        public ACService(ITableService<ACCurrent> service)
        {
            _service = service;
        }

        public async Task<ReportModel> GetReport()
        {
            var lastHourFrom = DateTime.Now.AddHours(-1);
            var now = DateTime.Now;

            var items = await _service.GetEntities(lastHourFrom, now);

            return new ReportModel
            {
                LastHourAverage = items.Any() ? items.Select(s => s.AC).Average() : 0
            };
        }
    }
}
