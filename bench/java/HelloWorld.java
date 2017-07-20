package org.eclipse.jetty.embedded;

import java.io.IOException;

import javax.servlet.ServletException;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

import org.eclipse.jetty.client.HttpClient;
import org.eclipse.jetty.server.Request;
import org.eclipse.jetty.server.Server;
import org.eclipse.jetty.server.handler.AbstractHandler;
import org.eclipse.jetty.client.api.*;
import org.eclipse.jetty.util.log.*;

public class HelloWorld extends AbstractHandler {
    private static HttpClient httpClient = new HttpClient();

    @Override
    public void handle(String target, Request baseRequest, HttpServletRequest request,
        HttpServletResponse response) throws IOException, ServletException {
        response.setContentType("text/html; charset=utf-8");
        response.setStatus(HttpServletResponse.SC_OK);

        try {
            ContentResponse httpResponse = httpClient.GET("http://localhost:3000");

            response.getWriter().println(httpResponse.getContent().toString());
        } catch (Exception e) {
            response.getWriter().println("error");
        }

        baseRequest.setHandled(true);
    }

    public static void main(String[] args) throws Exception {
        httpClient.start();

        System.setProperty("org.eclipse.jetty.LEVEL", "OFF");

        Server server = new Server(8080);
        server.setHandler(new HelloWorld());

        server.start();
        server.join();
    }
}
