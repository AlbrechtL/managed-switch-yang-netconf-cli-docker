import { Component, inject } from '@angular/core';
import { BreakpointObserver, Breakpoints } from '@angular/cdk/layout';
import { AsyncPipe } from '@angular/common';
import { MatToolbarModule } from '@angular/material/toolbar';
import { MatButtonModule } from '@angular/material/button';
import { MatSidenavModule } from '@angular/material/sidenav';
import { MatListModule } from '@angular/material/list';
import { MatIconModule } from '@angular/material/icon';
import { MatTooltipModule} from '@angular/material/tooltip';
import { MatSnackBar } from '@angular/material/snack-bar';
import { Observable } from 'rxjs';
import { map, shareReplay } from 'rxjs/operators';
import { MatSlideToggleModule } from '@angular/material/slide-toggle';
import { RouterOutlet, RouterModule } from '@angular/router';

@Component({
  selector: 'app-side-navigation',
  templateUrl: './side-navigation.component.html',
  styleUrl: './side-navigation.component.scss',
  standalone: true,
  imports: [
    MatToolbarModule,
    MatButtonModule,
    MatSidenavModule,
    MatListModule,
    MatIconModule,
    AsyncPipe,
    MatSlideToggleModule,
    RouterOutlet,
    RouterModule,
    MatTooltipModule,
  ]
})
export class SideNavigationComponent {
  private breakpointObserver = inject(BreakpointObserver);
  private _snackBar = inject(MatSnackBar);

  isHandset$: Observable<boolean> = this.breakpointObserver.observe(Breakpoints.Handset)
    .pipe(
      map(result => result.matches),
      shareReplay()
    );

  urlPrefix: string = '.'; // Just for development

  constructor() {}

  // commitSettings() {
  //   console.log("Committing settings...");
  // }

  saveSettings() {
    const url = `${this.urlPrefix}/rest/restconf/operations/ietf-netconf:copy-config`;
    const payload = {
      "ietf-netconf:input": {
        "target": { "startup": {} },
        "source": { "running": {} }
      }
    };

    fetch(url, {
      method: 'POST',
      headers: { 'Content-Type': 'application/yang-data+json' },
      body: JSON.stringify(payload)
    })
    .then(async res => {
      if (!res.ok) {
        const text = await res.text().catch(() => '');

        var div = document.createElement("div");
        div.innerHTML = text;
        console.error('Failed to save settings', res.status, div.innerText);
        this._snackBar.open(
          'Failed to save settings: error code ": ' + res.status + '" error text:"' + div.innerText + '"', 
          'Dismiss', 
          { panelClass: ['red-snackbar']}
        );
        return;
      }
      const data = await res.json().catch(() => null);
      console.log('Settings saved', data);
      this._snackBar.open('Settings saved', 'Dismiss', { duration: 2000});      
    })
    .catch(err => {
      console.error('Network error saving settings', err);
      this._snackBar.open('Network error saving settings', 'Dismiss', { panelClass: ['mat-warn']});
    });
  }
}
