import { Routes } from '@angular/router';
import { ConsoleComponent } from './console/console.component';
import { InfoComponent } from './info/info.component';
import { DashboardComponent } from './dashboard/dashboard.component';
import { VlanComponent } from './vlan/vlan.component';
import { ApiComponent } from './api/api.component';

export const routes: Routes = [
  { // Redirect to 'status' by default
    path: '',
    redirectTo: 'dashboard',
    pathMatch: 'full'
  },
  {
    path: 'dashboard',
    component: DashboardComponent,
    title: 'Dashboard'
  },
  {
    path: 'vlan',
    component: VlanComponent,
    title: 'VLAN'
  },
  {
    path: 'console',
    component: ConsoleComponent,
    title: 'Console'
  },
  {
    path: 'api',
    component: ApiComponent,
    title: 'API'
  },
  {
    path: 'info',
    component: InfoComponent,
    title: 'info'
  },
];
