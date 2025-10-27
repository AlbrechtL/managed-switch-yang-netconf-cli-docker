import { Component } from '@angular/core';
import { MatGridListModule } from '@angular/material/grid-list';
import { MatMenuModule } from '@angular/material/menu';
import { MatIconModule } from '@angular/material/icon';
import { MatButtonModule } from '@angular/material/button';
import { MatCardModule } from '@angular/material/card';
import { MatProgressSpinnerModule } from '@angular/material/progress-spinner';
import { MatListModule } from '@angular/material/list';
import { BackendCommunicationService } from '../backend-communication.service';


@Component({
  selector: 'app-dashboard',
  templateUrl: './dashboard.component.html',
  styleUrl: './dashboard.component.scss',
  standalone: true,
  imports: [
    MatGridListModule,
    MatMenuModule,
    MatIconModule,
    MatButtonModule,
    MatCardModule,
    MatProgressSpinnerModule,
    MatListModule
  ]
})
export class DashboardComponent {
  ports?: string[];

  ethernetInterfaces?: any;

  constructor(private service: BackendCommunicationService) {
    const tmpPorts = [];
    tmpPorts.push("dummy port 1")
    tmpPorts.push("dummy port 2")

    this.ports = tmpPorts;
  }
}
