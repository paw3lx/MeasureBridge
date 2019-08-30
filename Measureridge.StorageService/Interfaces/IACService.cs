using MeasureBridge.Model.Report;
using System;
using System.Collections.Generic;
using System.Text;
using System.Threading.Tasks;

namespace Measureridge.StorageService.Interfaces
{
    public interface IACService
    {
        Task<ReportModel> GetReport();
    }
}
