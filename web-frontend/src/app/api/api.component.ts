import { Component, AfterContentInit } from '@angular/core';
import { SwaggerUIBundle, SwaggerUIStandalonePreset } from "swagger-ui-dist"
import customerApiDoc from './restconf.json';

@Component({
  selector: 'app-api',
  imports: [],
  templateUrl: './api.component.html',
  styleUrl: './api.component.scss'
})
export class ApiComponent implements AfterContentInit {
  ngAfterContentInit(): void {
    //const apiDocumentation = customerApiDoc;
    const ui = SwaggerUIBundle({
      spec: customerApiDoc,
      domNode: document.getElementById('swagger-ui'),
    })
  }
}
