from django.conf.urls import patterns, include, url
from rest_framework import routers
import apiviews
import settings

# Uncomment the next two lines to enable the admin:
from django.contrib import admin
admin.autodiscover()

router = routers.DefaultRouter()
router.register(r'users', apiviews.UserViewSet)
router.register(r'jobs/stampede',apiviews.StampedeJobsViewSet,'job')
router.register(r'jobs/lonestar',apiviews.LonestarJobsViewSet,'ls4job')

urlpatterns = patterns('',
    # Examples:
    url(r'^$', 'tacc_stats_site.views.home', name='home'),
    url(r'^media/(?P<path>.*)$', 'django.views.static.serve',
        {'document_root': settings.MEDIA_ROOT}),
    # url(r'^tacc_stats_site/', include('tacc_stats_site.foo.urls')),

    # Uncomment the admin/doc line below to enable admin documentation:
    # url(r'^docs/admin/', include('django.contrib.admindocs.urls')),

     url(r'^docs/', include('rest_framework_swagger.urls')),

    # Uncomment the next line to enable the admin:
    url(r'^lonestar/', include('lonestar.urls', namespace="lonestar"),name='lonestar'),
    url(r'^stampede/', include('stampede.urls', namespace="stampede"),name='stampede'),

    url(r'^admin/', include(admin.site.urls)),

    #Django Rest API
    url(r'^api/', include(router.urls)),
    url(r'^api-auth/', include('rest_framework.urls', namespace='rest_framework')),
)
