#!/usr/bin/env node

require('proof')(2, function (step, equal) {
    step(function () {
        require('verity').createServer('api-key', step())
    }, function (verity) {
        step(function () {
            verity.connect('chrome', step())
        }, function (chrome) {
            step(function () {
                chome.open('https://www.prettyrobots.com/form', step())
            }, function () {
                browser.run(function ($, syn) {
                    $('input.name').val('Alan Gutierrez')
                    $('input.email').val('alan@prettyrobots.com')
                    $('input.submit').click()
                    return true
                }, step())
            }, function (clicked) {
                ok(clicked, 'clicked')
                browser.until(function () {
                    return /thank-you.html$/.test(window.location)
                }, step())
            }, function () {
                browser.run(function ($) {
                    return /Alan Gutierrez/.test($('div.message').val())
                }, step())
            }, function (thanked) {
                ok(thanked, 'thanked')
            }, function () {
                chrome.close(step())
            })
        }, function () {
            verity.close(step())
        })
    })
})
