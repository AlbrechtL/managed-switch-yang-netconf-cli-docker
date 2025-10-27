import { Injectable } from '@angular/core';
import { HttpClient } from '@angular/common/http';
import { Observable, tap, map } from 'rxjs';

@Injectable({
  providedIn: 'root'
})
export class BackendCommunicationService {
  //urlPrefix: string = 'http://localhost:8006'; // Just for development
  urlPrefix: string = '.'; // Relative URL

  constructor(private http: HttpClient) { }
}
